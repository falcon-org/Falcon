/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include "fs.h"
#include "exceptions.h"
#include "logging.h"

namespace falcon { namespace fs {

bool mkdir(const std::string& path) {
  struct stat sb;
  if (stat(path.c_str(), &sb) != 0) {
    if (::mkdir(path.c_str(), 0777) != 0) {
      LOG(ERROR) << "Cannot create directory " << path;
      return false;
    }
    return true;
  }

  if (!S_ISDIR(sb.st_mode)) {
    LOG(ERROR) << "Cannot create directory " << path << " because a file "
                  "with the same name exists";
    return false;
  }

  return true;
}

std::string dirname(const std::string& path) {
  std::string::size_type slash_pos = path.find_last_of("/");
  if (slash_pos == std::string::npos) {
    return std::string();
  }
  /* Strip any trailing slash. */
  while (slash_pos > 0 && path[slash_pos - 1] == '/') {
    slash_pos--;
  }
  return path.substr(0, slash_pos);
}

bool createPath(const std::string& path) {
  std::string dir = dirname(path);
  if (dir.empty()) {
    return true;
  }

  struct stat sb;
  if (stat(dir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    /* Path already exists. */
    return true;
  }

  if (!createPath(dir)) {
    return false;
  }

  return mkdir(dir);
}

} } //namespace falcon::fs
