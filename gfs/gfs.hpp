/*
gfs - Minimal experimental archive format
Version 0.0.1

Copyright (c) 2026 Lucas

Licensed under the MIT License.
See LICENSE file for details.
*/
#ifndef GFS_HPP
#define GFS_HPP

#include <cstdint>
#include <string>

/*
If anyone ever wants to read this code instead of enjoying life, I'd better explain the terminology :
- GFS : well, it's the file format (name of the repo...).
- Content file : any file stored within a gfs file.
*/

namespace gfs {

#define GFS_VERSION_MAJOR 0
#define GFS_VERSION_MINOR 0
#define GFS_VERSION_PATCH 1

inline constexpr uint32_t GFS_MAGIC = 0x47465330; //GFS0

struct Header {
  uint32_t magic;
  uint32_t versionMajor;
  uint32_t versionMinor;
  uint32_t fileCount;
};

struct ContentFileInfo {
  std::string virtualPath;
  uint64_t size;
  uint64_t offset;
};

struct IndexFile {
  uint64_t size;
  uint64_t offset;
  uint32_t path_length;
  // followed immediatly by the path bytes in the file
};

}

#endif