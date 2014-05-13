/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#include "cache_fs.h"

#include "fs.h"
#include "logging.h"

namespace falcon {

CacheFS::CacheFS(const std::string& dir)
    : dir_(dir) {}

bool CacheFS::writeEntry(const std::string& hash, const std::string& path) {
  assert(!hash.empty());

  fs::mkdir(dir_);

  std::string output = dir_;
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

bool CacheFS::hasEntry(const std::string& hash) {
  assert(!hash.empty());
  std::string output = dir_;
  output.append("/");
  output.append(hash);
  struct stat sb;
  return stat(output.c_str(), &sb) == 0;
}

bool CacheFS::readEntry(const std::string& hash, const std::string& path) {
  assert(!hash.empty());
  std::string output = dir_;
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

bool CacheFS::delEntry(const std::string& hash) {
  assert(!hash.empty());
  std::string entry = dir_;
  entry.append("/");
  entry.append(hash);

  struct stat sb;
  if (stat(entry.c_str(), &sb) != 0) {
    return true;
  }

  if (unlink(entry.c_str()) < 0) {
    LOG(ERROR) << "Could not remove " << entry;
    return false;
  }

  return true;
}

} // namespace falcon
