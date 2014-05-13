/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_FS_H_
#define FALCON_FS_H_

#include <string>

namespace falcon { namespace fs {

/**
 * Create a directory if it does not already exist.
 * @param path of the directory.
 * @return true on success, false on error.
 */
bool mkdir(const std::string& path);

/**
 * Ensure the hierarchy of directories exists for the given path.
 */
bool createPath(const std::string& path);

/**
 * Retrieve the directory name of a path.
 * Ex: dirname("/path/to/a/file") returns "/path/to/a"
 *     dirname("/path/to/a") returns "/path/to"
 */
std::string dirname(const std::string& path);

} } //namespace falcon::fs

#endif // FALCON_FS_H_
