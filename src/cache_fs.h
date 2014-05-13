/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */
#ifndef FALCON_CACHE_FS_H_
#define FALCON_CACHE_FS_H_

#include <string>

namespace falcon {

class CacheFS {
 public:

  CacheFS(const std::string& dir);

  /**
   * Write an entry in the cache.
   * @param hash of the entry.
   * @param path of the entry.
   * @return true on sucess, false otherwise.
   */
  bool writeEntry(const std::string& hash, const std::string& path);

  /**
   * Check if the cache contains an entry.
   * @param hash Hash of the entry.
   * @return True if the entry exists, false otherwise.
   */
  bool hasEntry(const std::string& hash);

  /**
   * Query the cache for an entry with the given hash and restore it to the
   * given path.
   * @param hash Hash of the entry.
   * @param path Path where to store the entry.
   * @return true if the entry was found, false otherwise.
   */
  bool readEntry(const std::string& hash, const std::string& path);

  /**
   * Remove the given entry from cache.
   * @param hash of the entry.
   * @return true if entry was removed or did not exist, false on error.
   */
  bool delEntry(const std::string& hash);

 private:
  std::string dir_;
};

} // namespace falcon

#endif // FALCON_CACHE_FS_H_
