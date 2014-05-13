/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <sstream>
#include <sys/stat.h>

#include "daemon_instance.h"

#include "command_server.h"
#include "exceptions.h"
#include "graph_consistency_checker.h"
#include "graph_hash.h"
#include "graph_parallel_builder.h"
#include "graph_reloader.h"
#include "graphparser.h"
#include "logging.h"
#include "watchman.h"

using namespace std::placeholders;

namespace falcon {

DaemonInstance::DaemonInstance(std::unique_ptr<GlobalConfig> gc,
                               std::unique_ptr<CacheManager> cache)
    : buildId_(0)
    , config_(std::move(gc))
    , watchmanClient_(config_->getWorkingDirectoryPath())
    , isBuilding_(false)
    , streamServer_()
    , cache_(std::move(cache)) { }

void DaemonInstance::loadConf(std::unique_ptr<Graph> gp) {
  graph_ = std::move(gp);
}

void DaemonInstance::start() {
  assert(!config_->runSequentialBuild());
  assert(!commandServer_);
  assert(graph_);

  /* Register all source files to watchman. */
  try {
    watchmanClient_.watchGraph(*graph_);
  } catch (falcon::Exception& e) {
    LOG(ERROR) << e.getErrorMessage();
  }

  /* Open the stream server's socket and accept clients in another thread. */
  streamServer_.openPort(config_->getNetworkStreamPort());
  std::thread streamServerThread = std::thread(&StreamServer::run,
                                               &streamServer_);

  /* Start the server. This will block until the server shuts down. */
  LOG(INFO) << "Starting server...";
  commandServer_.reset(new CommandServer(this, config_->getNetworkAPIPort()));
  commandServer_->start();

  /* If we reach here, the server was shut down. */
  if (builder_) {
    builder_->wait();
  }
  streamServer_.stop();
  streamServerThread.join();
}

void DaemonInstance::checkSourcesMissing() {
  if (!sourcesMissing_.empty()) {
    std::ostringstream oss;
    oss << "Error, cannot build without: ";
    bool first = true;
    for (auto it = sourcesMissing_.begin(); it != sourcesMissing_.end(); ++it) {
      if (first) {
        first = false;
      } else {
        oss << ", ";
      }
      oss << (*it)->getPath();
    }
    InvalidGraphError e;
    e.desc = oss.str();
    throw e;
  }
}

/* Commands */

StartBuildResult::type DaemonInstance::startBuild(int32_t numThreads) {
  assert(graph_);

  if (isBuilding_.load(std::memory_order_acquire)) {
     return StartBuildResult::BUSY;
  }

  if (cache_->getPolicy() == CacheManager::Policy::CACHE_GIT_REFS) {
    cache_->gitUpdateRef();
  }

  checkSourcesMissing();

  FALCON_CHECK_GRAPH_CONSISTENCY(graph_.get(), mutex_);

  isBuilding_.store(true, std::memory_order_release);

  streamServer_.newBuild(buildId_);

  /* Create a build plan that builds everything.
   * TODO: the user should be able to explicitly give the targets to build. */
  plan_.reset(new BuildPlan(graph_->getRoots()));

  auto callback = std::bind(&DaemonInstance::onBuildCompleted, this, _1);
  builder_.reset(
      new GraphParallelBuilder(*graph_, *plan_, cache_.get(), &streamServer_,
                               &watchmanClient_,
                               config_->getWorkingDirectoryPath(),
                               numThreads, mutex_, callback));
  builder_->startBuild();

  return StartBuildResult::OK;
}

void DaemonInstance::onBuildCompleted(BuildResult res) {
  assert(isBuilding_);
  streamServer_.endBuild(res);

  FALCON_CHECK_GRAPH_CONSISTENCY(graph_.get(), mutex_);

  ++buildId_;
  isBuilding_.store(false, std::memory_order_release);

  LOG(INFO) << "Build completed. Status: " << toString(res);
}

FalconStatus::type DaemonInstance::getStatus() {
  return isBuilding_ ? FalconStatus::BUILDING : FalconStatus::IDLE;
}

void DaemonInstance::interruptBuild() {
  LOG(INFO) << "Interrupting build.";
  if (builder_) {
    builder_->interrupt();
  }
}

void DaemonInstance::getDirtySources(std::set<std::string>& sources) {
  lock_guard g(mutex_);

  NodeSet& src = graph_->getSources();
  for (auto it = src.begin(); it != src.end(); ++it) {
    if ((*it)->getState() == State::OUT_OF_DATE) {
      sources.insert((*it)->getPath());
    }
  }
}

void DaemonInstance::getDirtyTargets(std::set<std::string>& targets) {
  lock_guard g(mutex_);

  NodeMap& nodes = graph_->getNodes();
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (it->second->getState() == State::OUT_OF_DATE) {
      targets.insert(it->second->getPath());
    }
  }
}

void DaemonInstance::getInputsOf(std::set<std::string>& inputs,
                                 const std::string& target) {
  lock_guard g(mutex_);

  auto it = graph_->getNodes().find(target);
  if (it == graph_->getNodes().end()) {
    throw TargetNotFound();
  }

  Rule* rule = it->second->getChild();
  if (rule != nullptr) {
    auto ins = rule->getInputs();
    for (auto itInput = ins.begin(); itInput != ins.end(); ++itInput) {
      inputs.insert((*itInput)->getPath());
    }
  }
}

void DaemonInstance::getOutputsOf(std::set<std::string>& outputs,
                                  const std::string& target) {
  lock_guard g(mutex_);

  auto it = graph_->getNodes().find(target);
  if (it == graph_->getNodes().end()) {
    throw TargetNotFound();
  }

  auto rules = it->second->getParents();
  for (auto itRule = rules.begin(); itRule != rules.end(); ++itRule){
    auto outs = (*itRule)->getOutputs();
    for (auto itOutput = outs.begin(); itOutput != outs.end(); ++itOutput) {
      outputs.insert((*itOutput)->getPath());
    }
  }
}

void DaemonInstance::getHashOf(std::string& hash, const std::string& target) {
  lock_guard g(mutex_);

  auto it = graph_->getNodes().find(target);
  if (it == graph_->getNodes().end()) {
    throw TargetNotFound();
  }

  hash = it->second->getHash();
}

void DaemonInstance::setDirty(const std::string& target) {
  lock_guard g(mutex_);

  if (target == config_->getJsonGraphFile()) {
    reloadGraph();
    return;
  }

  /* Find the target. */
  auto& map = graph_->getNodes();
  auto it = map.find(target);
  if (it == map.end()) {
    throw TargetNotFound();
  }

  Node* node = it->second;
  if (!node->isSource() && node->getChild()->isPhony()) {
    /* This is a phony target. */
    node->markDirty();
    return;
  }

  /* Stat the node. */
  struct stat st;
  if (stat(node->getPath().c_str(), &st) < 0) {
    if (errno != ENOENT && errno != ENOTDIR) {
      LOG(WARNING) << "Failed to stat Node '" << node->getPath() << "'";
      DLOG(WARNING) << "stat(" << node->getPath()
                    << ") [" << errno << "] " << strerror(errno);
    }
    if (node->isSource() && node->isExplicitDependency()) {
      /* It is a source file, and we cannot stat it. This is an error and the
       * next build will fail.
       * Note: we don't mark it as an error if the node is not an explicit
       * depedency, ie it is only an implicit dependency of other nodes. In that
       * case, we can't know for sure that the file being missing is an error.
       */
      LOG(ERROR) << "Error: input file " << node->getPath() << " is missing.";
      sourcesMissing_.insert(node);
    }
  } else {
    if (node->isSource()) {
      /* This is a source file and it is not missing. */
      sourcesMissing_.erase(node);
    } else {
      /* This node is an output. Compare its timestamp with the timestamp of the
       * rule that generated it. If it is younger, it means we are notified
       * because the file was built by it, so don't mark it dirty. */
      if (node->getChild()->getTimestamp() >= st.st_mtime) {
        return;
      }
    }
  }

  /**
   * Mark the node dirty if:
   * - it if not a source file;
   * - it is a source file and the hash does not change (ie it *really*
   *   changed).
  */
  if (!node->isSource() || hash::recomputeNodeHash(node, &watchmanClient_,
                                                   *graph_, cache_.get(),
                                                   true, true)) {
    node->markDirty();
    /* If this node is generated by a rule, mark it dirty as well because it
     * needs to re-run. */
    if (!node->isSource()) {
      node->getChild()->setState(State::OUT_OF_DATE);
    }
  } else {
    LOG(INFO) << "Not marking " << node->getPath();
  }
}

void DaemonInstance::shutdown() {
  LOG(INFO) << "Shutting down.";

  watchmanClient_.unwatchGraph(*graph_);

  /* Interrupt the current build. */
  interruptBuild();

  /* Stop the thrift server. */
  assert(commandServer_);
  /* TODO: is this a problem to call stop from a thrift handler? */
  commandServer_->stop();
}

void DaemonInstance::getGraphviz(std::string& str) {
  lock_guard g(mutex_);

  assert(graph_);
  std::ostringstream oss;
  printGraphGraphiz(*graph_, oss);
  str = oss.str();
}

void DaemonInstance::reloadGraph() {
  GraphParser graphParser(config_->getJsonGraphFile());

  try {
    graphParser.processFile();
  } catch (Exception& e) {
    LOG(ERROR) << e.getErrorMessage ();
    return;
  }

  std::unique_ptr<Graph> graphPtr = std::move(graphParser.getGraph());
  try {
    checkGraphLoop(*graphPtr);
  } catch (Exception& e) {
    LOG(ERROR) << e.getErrorMessage();
    delete graphPtr.release();
    return;
  }

  GraphReloader reloader(graph_.release(), graphPtr.release(), watchmanClient_,
                         cache_.get());
  graph_ = std::unique_ptr<Graph>(reloader.getUpdatedGraph());
}

} // namespace falcon
