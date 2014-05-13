/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_PARALLEL_BUILDER_H_
#define FALCON_GRAPH_PARALLEL_BUILDER_H_

#include <atomic>
#include <memory>
#include <thread>

#include "build_plan.h"
#include "cache_manager.h"
#include "graph.h"
#include "graph_builder.h"
#include "posix_subprocess_manager.h"
#include "watchman.h"

namespace falcon {

class GraphParallelBuilder : public IGraphBuilder {
 public:
  GraphParallelBuilder(Graph& Graph,
                       BuildPlan& plan,
                       CacheManager* cache,
                       IStreamConsumer* consumer,
                       WatchmanClient* watchmanClient,
                       std::string const& workingDirectory,
                       std::size_t numThreads,
                       std::mutex& mutex,
                       onBuildCompletedFn callback);

  ~GraphParallelBuilder();

  void startBuild();

  void interrupt();

  void wait();

  BuildResult getResult() const { return result_; }

 private:
  void buildThread();
  void buildRule(Rule *rule);
  bool tryBuildRuleFromCache(Rule *rule);
  void markOutputsUpToDate(Rule *rule);
  BuildResult waitForNext();
  void onRuleFinished(Rule* rule);

  Graph& graph_;
  BuildPlan& plan_;
  CacheManager* cache_;
  IStreamConsumer* consumer_;
  PosixSubProcessManager manager_;
  WatchmanClient * watchmanClient_;
  std::string workingDirectory_;
  std::size_t numThreads_;
  BuildResult result_;

  std::unique_lock<std::mutex> lock_;
  std::atomic_bool interrupted_;
  onBuildCompletedFn callback_;
  std::thread thread_;
};

} // namespace falcon

#endif // FALCON_GRAPH_PARALLEL_BUILDER_H_
