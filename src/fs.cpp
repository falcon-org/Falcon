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


} } //namespace falcon::fs
