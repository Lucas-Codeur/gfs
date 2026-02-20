# GFS - Game File System
"Game File System" is an experimental, fast read-only archive format project intended for learning purposes and later use in games. This first implementation is C++ but the format can be implemented using any programming language.

It is currently in early development so it may still be unstable, feel free to report a bug in the issues section.

## Intent
The goal is to create a simple archive format, fast to read with random access that can easily manage thousands of relatively small files to simplify resource management in games and other apps in general.
## Roadmap (near-future) :
- GFS 0.1 ✅
- GFS CLI 0.1 ✅
- Finish readme🟠 (quick start, how to use...)
- Add tests and exemples 🔜
- Improve compatibility with other compilers and linux 🔜
- Add benchmark 🔜

---
# Format
The **GFS 0.1** binary file format is composed of three parts : Header, Content and Index. All numbers use big-endian notation. There is no padding or alignment anywhere in the format.

## Header
The header is the first byte sequence of the file : it contains the following informations, in the same order. And it is 24 bytes long.

| Magic   | Version Major | Version Minor | File count | Index offset |
|---------|---------------|---------------|------------|--------------|
| uint32  | uint32        | uint32        | uint32     | uint64       |
| 4 bytes | 4 bytes       | 4 bytes       | 4 bytes    | 8 bytes      |

The "magic" is used to identify GFS files, its value should always remain **0x47465330**. It is the four ascii bytes `G`,`F`,`S`,`0`, packed into a big-endian uint32.

Index offset is the absolute offset, from file start, of the index.

## Content
Files are stored continuously directly after the header (byte 24). 

To read a specific file, seek to its offset as recorded in the index and read size bytes

## Index

The index is the last part of the file, written directly after the content, it must contain **File Count** (from header) byte sequences using this format :

| Size   | Offset | Path length | Path bytes         |
|--------|--------|-------------|--------------------|
| uint64 | uint64 | uint32      | char [path length] |

Because path can vary a lot in size, we store the path length. Offset is absolute from file start and identify where to find the file byte sequence in the content section. And size is the number of bytes from the offset which make up the file content.

To protect from large memory allocation in case of corrupt data, the max path length is 2048. 
For now, paths must be valid UTF-8.