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

inline constexpr unsigned int GFS_HEADER_SIZE = 4 * sizeof(uint32_t) + 1 * sizeof(uint64_t);

struct Header {
  uint32_t magic;
  uint32_t versionMajor;
  uint32_t versionMinor;
  uint32_t fileCount;
  uint64_t indexOffset; // The index is written after the data
};

struct IndexFile {
  uint64_t size;
  uint64_t offset;
  uint32_t path_length;
  // followed immediatly by the path bytes in the file
};

struct IndexEntry {
	uint64_t size = 0;
	uint64_t offset = 0;
	std::string virtualPath;
	// followed immediatly by the path bytes in the file
};

struct Archive {
  std::string path;
  uint32_t fileCount;
  uint32_t versionMajor;
  uint32_t versionMinor;
};

struct SourceFile {
  std::string sourcePath;
  std::string virtualPath;
  uint64_t offset = GFS_UNDEFINED;
  uint64_t size = GFS_UNDEFINED;
};

bool createArchive(const std::string& outputPath, const std::vector<SourceFile>& files);

namespace internal {
  inline void writeHeaderPlaceholder(std::ostream& output);
  inline void writeIndex(const std::vector<IndexEntry>& index, std::ostream& output);
  inline std::vector<IndexEntry> writeContent(const std::vector<SourceFile>& files, std::ostream& output);

  inline void writeU32(std::ostream& output, uint32_t value);
  inline void writeU64(std::ostream& output, uint64_t value);
}

// Implementation

bool createArchive(const std::string& outputPath, const std::vector<SourceFile>& files) {
  std::ofstream file(outputPath, std::ios::binary);
  if(!file.good()) return GFS_FAILED;
  
  file.clear(); // We work with offset math so the file better be empty
  internal::writeHeaderPlaceholder(file); // Header is the first byte sequence
  std::vector<IndexEntry> index = internal::writeContent(files, file);
  std::cout << "Index size : " << index.size();

  uint64_t indexPos = file.tellp();
  internal::writeIndex(index, file);

  if (!file) return GFS_FAILED;
  file.seekp(GFS_HEADER_SIZE - sizeof(uint64_t), std::ios::beg);   // go to byte offset 8
  internal::writeU64(file, indexPos);
  
  return GFS_SUCCESS;
}

void internal::writeHeaderPlaceholder(std::ostream& output) {
	internal::writeU32(output, GFS_MAGIC);
	internal::writeU32(output, GFS_VERSION_MAJOR);
	internal::writeU32(output, GFS_VERSION_MINOR);

	// These fields will be filled after content and index
	internal::writeU32(output, UINT32_MAX); // File count
	internal::writeU64(output, UINT64_MAX); // Index offset
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

void internal::writeIndex(const std::vector<IndexEntry>& index, std::ostream& output) {
	for (auto& entry : index) {
		std::cout << entry.offset << std::endl;

		writeU64(output, entry.size);
		writeU64(output, entry.offset);

		uint32_t pathLength = static_cast<uint32_t>(entry.virtualPath.size());
		writeU32(output, pathLength);

		output.write(entry.virtualPath.data(), pathLength);
	}
}

std::vector<IndexEntry> internal::writeContent(const std::vector<SourceFile>& files, std::ostream& output) {
	std::vector<IndexEntry> index;

	for (auto& file : files) {
		std::ifstream source(file.sourcePath.c_str(), std::ifstream::binary);
		
		if (!source || !source.is_open()) {
			std::cerr << "Could not find file " << file.sourcePath << std::endl;
			continue;
		}
		IndexEntry entry;
		entry.offset = static_cast<uint64_t>(output.tellp());
		entry.virtualPath = file.virtualPath;

		std::vector<char> buffer(4096);
		while (source) {
			source.read(buffer.data(), buffer.size());
			std::streamsize readBytes = source.gcount();

			entry.size += static_cast<uint64_t>(readBytes);
			if (readBytes > 0) {
				output.write(buffer.data(), readBytes);
			}
		}

		index.push_back(entry);
	}

	return index;
}

}

#endif