/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "cache_manager.h"

#include "exceptions.h"
#include "logging.h"
#include "fs.h"

namespace falcon {

CacheManager::CacheManager(const std::string& falconDir)
    : localCacheDir_(falconDir) {
  localCacheDir_.append("/cache");
}

bool CacheManager::has(const std::string& hash) {
  assert(!hash.empty());
  std::string output = localCacheDir_;
  output.append("/");
  output.append(hash);
  struct stat sb;
  return stat(output.c_str(), &sb) == 0;
}

bool CacheManager::read(const std::string& hash, const std::string& path) {
  assert(!hash.empty());
  std::string output = localCacheDir_;
  output.append("/");
  output.append(hash);

  struct stat sb;
  if (stat(output.c_str(), &sb) != 0) {
    return false;
  }

  /* Copy the target from the cache. */
  try {
    std::ifstream ifs(output, std::ios::binary);
    std::ofstream ofs(path, std::ios::binary);
    ofs << ifs.rdbuf();
  } catch (std::ios_base::failure& e) {
    LOG(ERROR) << "Could not retrieve " << path << " in cache: " << e.what();
    return false;
  }

  return true;
}

bool CacheManager::update(const std::string& hash, const std::string& path) {
  assert(!hash.empty());

  fs::mkdir(localCacheDir_);

  std::string output = localCacheDir_;
  output.append("/");
  output.append(hash);

  struct stat sb;
  if (stat(output.c_str(), &sb) == 0) {
    /* The target is already in cache. */
    return true;
  }

  /* Copy the target in the cache. */
  try {
    std::ifstream ifs(path, std::ios::binary);
    std::ofstream ofs(output, std::ios::binary);
    ofs << ifs.rdbuf();
  } catch (std::ios_base::failure& e) {
    LOG(ERROR) << "Could not store " << path << " in cache: " << e.what();
    return false;
  }

  return true;
}

} // namespace falcon
