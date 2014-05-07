/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_CACHE_MANAGER_H_
#define FALCON_CACHE_MANAGER_H_

#include <string>

namespace falcon {

class CacheManager {
 public:
  CacheManager(const std::string& workingDirectory);

  /**
   * Check if the cache contains an entry.
   * @param hash Hash of the entry.
   * @return True if the entry exists, false otherwise.
   */
  bool has(const std::string& hash);

  /**
   * Query the cache for a target with the given hash and store it to the given
   * path.
   * @param hash Hash of the target.
   * @param path Path where to store the target.
   * @return true if the target was found, false otherwise.
   */
  bool read(const std::string& hash, const std::string& path);

  /**
   * Update the cache for a target.
   * @param hash of the target.
   * @param path of the target.
   * @return true on sucess, false otherwise.
   */
  bool update(const std::string& hash, const std::string& path);

 private:
  std::string localCacheDir_;
};

} // namespace falcon

#endif // FALCON_CACHE_MANAGER_H_
