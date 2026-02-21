# GFS - Game File System
"Game File System" is an experimental, fast read-only archive format project intended for learning purposes and later use in games. This first implementation is C++ but the format can be implemented using any programming language.

It is currently in early development so it may still be unstable, feel free to report a bug in the issues section.

## Intent
The goal is to create a simple archive format, fast to read with random access that can easily manage thousands of relatively small files to simplify resource management in games and other apps in general.
## Roadmap (near-future) :
- Add tests and exemples 🔜
- Improve compatibility with other compilers and linux 🔜
- Add benchmark 🔜

---
# Quick start

The library is a single header file, simply copy `gfs/gfs.hpp` inside your project and you're done.

> [!TIP]
> The **cli code** contains relevent examples on how to use the lib, you can find it in the `gfs-cli` folder

Sample code to create an archive from a set of files :
```cpp
#define GFS_ENABLE_DEBUG // Used to enable debug, use before gfs include
#include <gfs.hpp>

int main() {
	std::vector<gfs::SourceFile> files;
    // Add file source and it's name in the archive
	files.push_back({ "C:\\Users\\<your user>\\game\\game_sound.mp3", "sound1.mp3" });
	files.push_back({ "C:\\Users\\<your user>\\game\\texture_intro.png", "intro.png" });
	files.push_back({ "C:\\Users\\<your user>\\game\\shader_vert.glsl", "shaders/shader.vert.glsl" });

    // Create an archive at the given location
	gfs::GfsResult result = gfs::createArchive("C:\\Users\\<your user>\\game\\assets.gfs", files);
    if(result != gfs::GfsResult::SUCCESS) {
        exit(-1);
    }
	
	return 0;
}
```

Other sample code to retrieve file data from an archive :

```cpp
#define GFS_ENABLE_DEBUG // Used to enable debug, use before gfs include
#include <gfs.hpp>

int main() {
	gfs::Archive gameAssets;
	gfs::GfsResult result = gfs::readArchive("C:\\Users\\<your user>\\game\\assets.gfs", gameAssets); // Loads the header and the index
    if(result != gfs::GfsResult::SUCCESS) {
        exit(-1);
    }

	std::ifstream assetsStream; // Open the archive input stream, you can reuse it for multiple operation
	result = gfs::openArchiveInputStream(gameAssets, assetsStream);
    if(result != gfs::GfsResult::SUCCESS) {
        exit(-1);
    }

	std::vector<char> data; // Extract data from a file as a byte array
	result = gfs::readArchivedFile("test.txt", assetsStream, gameAssets, data);
    if(result != gfs::GfsResult::SUCCESS) {
        exit(-1);
    }
	return 0;
}
```

---
# Format
The **GFS 0.2** binary file format is composed of three parts : Header, Content and Index. All numbers use little-endian notation. There is no padding or alignment anywhere in the format.

## Header
The header is the first byte sequence of the file : it contains the following informations, in the same order. And it is 24 bytes long.

| Magic   | Version Major | Version Minor | Flags   | File count | Index offset |
|---------|---------------|---------------|---------|------------|--------------|
| uint32  | uint32        | uint32        | uint32  | uint32     | uint64       |
| 4 bytes | 2 bytes       | 2 bytes       | 4 bytes | 4 bytes    | 8 bytes      |

The "magic" is used to identify GFS files, its value should always remain **0x47465330**. It is the four ascii bytes `G`,`F`,`S`,`0`, packed into a uint32.

Index offset is the absolute offset, from file start, of the index.

Flags are reserved for future use.

Version major is used for breaking changes and version minor for smaller, backward compatible changes. **However** until 1.0 releases minor versions are breaking changes.

## Content
Files are stored continuously directly after the header. 

To read a specific file, seek to its offset as recorded in the index and read size bytes

## Index

The index is the last part of the file, written directly after the content, it must contain **File Count** (from header) byte sequences using this format :

| Size   | Offset | Path length | Path bytes         |
|--------|--------|-------------|--------------------|
| uint64 | uint64 | uint32      | char [path length] |

Because path can vary a lot in size, we store the path length. Offset is absolute from file start and identify where to find the file byte sequence in the content section. And size is the number of bytes from the offset which make up the file content.

To protect from large memory allocation in case of corrupt data, the max path length is 2048. 
For now, paths must be valid UTF-8.

---

# Changelog
### GFS 0.2 - 21 feb 2026
- Changed big endian numbers to little endian
- Version major and minor are now each 16 bits instead of 32
- Added `flags` in the header for future use
- The implementation now properly checks for minor version breaking change (before 1.0)
- Added quickstart examples