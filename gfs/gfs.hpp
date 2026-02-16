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
#include <vector>
#include <fstream>

/*
If anyone ever wants to read this code instead of enjoying life, I'd better explain the terminology :
- GFS : well, it's the file format (name of the repo...).
- Content file : any file stored within a gfs file.
- Source file : a real file to be put into the archive.
- Virtual path : path of file inside the archive.
*/

// TODO remove this
#include <iostream>

namespace gfs {

#define GFS_UNDEFINED 0

inline constexpr uint32_t GFS_VERSION_MAJOR = 0;
inline constexpr uint32_t GFS_VERSION_MINOR = 1;
inline constexpr uint32_t GFS_VERSION_PATCH = 0;

inline constexpr uint32_t GFS_MAGIC = 0x47465330; // GFS0

inline constexpr bool GFS_SUCCESS = true;
inline constexpr bool GFS_FAILED = false;

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

struct Archive {
  std::string path;
  uint32_t fileCount;
  uint32_t versionMajor;
  uint32_t versionMinor;
  std::vector<ContentFileInfo> files;
};

struct SourceFile {
  std::string sourcePath;
  std::string virtualPath;
  uint64_t offset = GFS_UNDEFINED;
  uint64_t size = GFS_UNDEFINED;
};

bool createArchive(const std::string& outputPath, const std::vector<SourceFile>& files);

namespace internal {
  inline void writeHeader(size_t fileCount, std::ostream& output);
  inline void writeIndex(const std::vector<SourceFile>& files, std::ostream& output);
  inline void writeContent(const std::vector<SourceFile>& files, std::ostream& output);

  inline void writeU32(std::ostream& output, uint32_t value);
  inline void writeU64(std::ostream& output, uint64_t value);
}

// Implementation

bool createArchive(const std::string& outputPath, const std::vector<SourceFile>& files) {
  std::ofstream file(outputPath, std::ios::binary);
  if(!file.good()) return GFS_FAILED;

  file.clear(); // We work with offset math so the file better be empty
  internal::writeHeader(files.size(), file); // Header is the first byte sequence
  internal::writeIndex(files, file);
  
  return GFS_SUCCESS;
}

void internal::writeHeader(size_t fileCount, std::ostream& output) {
	internal::writeU32(output, GFS_MAGIC);
	internal::writeU32(output, GFS_VERSION_MAJOR);
	internal::writeU32(output, GFS_VERSION_MINOR);
	internal::writeU32(output, fileCount);
}

// Correct endianess issues related to directly write the magic
void internal::writeU32(std::ostream& output, uint32_t value) {
	unsigned char bytes[4];
	bytes[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
	bytes[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
	bytes[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
	bytes[3] = static_cast<unsigned char>(value & 0xFF);
	output.write(reinterpret_cast<char*>(bytes), sizeof(bytes));
}

void internal::writeU64(std::ostream& output, uint64_t value) {
	writeU32(output, (value >> 32) & 0xFFFFFFFFu);
	writeU32(output, value & 0xFFFFFFFFu);
}

void internal::writeIndex(const std::vector<SourceFile>& files, std::ostream& output) { }
void internal::writeContent(const std::vector<SourceFile>& files, std::ostream& output) { }

}

#endif