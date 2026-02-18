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

#ifdef GFS_ENABLE_DEBUG
#include <iostream>
#include <sstream>
#define GFS_DEBUG(msg)                                         \
    {                                                          \
        std::stringstream ss;                                  \
        ss << msg;                                             \
        std::cout << "[GFS DEBUG]: " << ss.str() << std::endl; \
    }
#else
#define GFS_DEBUG(msg)
#endif

namespace gfs
{
    inline constexpr uint32_t GFS_VERSION_MAJOR = 0;
    inline constexpr uint32_t GFS_VERSION_MINOR = 1;
    inline constexpr uint32_t GFS_VERSION_PATCH = 0; // Used for Cmake version

    inline constexpr uint32_t GFS_MAGIC = 0x47465330; // GFS0

    inline constexpr unsigned int GFS_HEADER_SIZE = 4 * sizeof(uint32_t) + 1 * sizeof(uint64_t);

    enum class GfsResult
    {
        SUCCESS = 0,
        OUTPUT_FILE_ERROR = 1,
        SOURCE_FILE_ERROR = 2,
        WRITE_ERROR = 3,
        INPUT_FILE_ERROR = 4,
        INVALID_DATA_ERROR = 5,
    };

    struct Header
    {
        uint32_t magic;
        uint32_t versionMajor;
        uint32_t versionMinor;
        uint32_t fileCount;
        uint64_t indexOffset; // The index is written after the data
    };

    struct IndexEntry
    {
        uint64_t size = 0;
        uint64_t offset = 0;
        std::string virtualPath;
        // followed immediatly by the path bytes in the file
    };

    using Index = std::vector<IndexEntry>;

    struct Archive
    {
        Header header;
        Index index;
    };

    struct SourceFile
    {
        std::string sourcePath;
        std::string virtualPath;
    };

    inline GfsResult createArchive(const std::string &outputPath, const std::vector<SourceFile> &files);
    inline GfsResult readArchive(const std::string &path, Archive& archive);

    namespace internal
    {
        // Write related stuff
        inline GfsResult writeHeaderPlaceholder(std::ostream &output);
        inline GfsResult writeContent(const std::vector<SourceFile> &files, std::ostream &output, std::vector<IndexEntry>& index);
        inline GfsResult writeIndex(const Index &index, std::ostream &output);

        inline void writeU32(std::ostream &output, uint32_t value);
        inline void writeU64(std::ostream &output, uint64_t value);

        // Read related stuff
        inline GfsResult readHeader(std::ifstream &input, Header& header);
        inline GfsResult readIndex(uint64_t indexPos, uint32_t fileCount, std::ifstream &input, Index& index);

        inline uint32_t readU32(std::ifstream &input);
        inline uint64_t readU64(std::ifstream &input);
    }
}

// Implementation
namespace gfs {
    inline GfsResult createArchive(const std::string& outputPath, const std::vector<SourceFile>& files)
    {
        GFS_DEBUG("Attempting to create archive " << outputPath);

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.good())
            return GfsResult::OUTPUT_FILE_ERROR;

        internal::writeHeaderPlaceholder(file);

        std::vector<IndexEntry> index;
        GfsResult stepResult = internal::writeContent(files, file, index);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        uint64_t indexPos = static_cast<uint64_t>(file.tellp());
        if (indexPos == std::streampos(-1))
            return GfsResult::WRITE_ERROR;

        stepResult = internal::writeIndex(index, file);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        if (!file)
            return GfsResult::WRITE_ERROR;

        uint32_t fileCount = static_cast<uint32_t>(index.size());
        file.seekp(GFS_HEADER_SIZE - sizeof(uint64_t) - sizeof(uint32_t), std::ios::beg);
        internal::writeU32(file, fileCount);

        file.seekp(GFS_HEADER_SIZE - sizeof(uint64_t), std::ios::beg);
        internal::writeU64(file, indexPos);

        GFS_DEBUG("Failed files: " << (files.size() - fileCount))
        GFS_DEBUG("Total files: " << fileCount)

        GFS_DEBUG("Archive created successfully");
        return GfsResult::SUCCESS;
    }

    inline GfsResult readArchive(const std::string& path, Archive& archive)
    {
        std::ifstream file(path.c_str(), std::ifstream::binary);
        if (!file || !file.is_open())
        {
            GFS_DEBUG("Error, could not open archive " << path);
            return GfsResult::INPUT_FILE_ERROR;
        }

        Header header;
        GfsResult stepResult = internal::readHeader(file, header);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        archive.header = header;
        stepResult = internal::readIndex(header.indexOffset, header.fileCount, file, archive.index);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::readHeader(std::ifstream& input, Header& header)
    {
        header.magic = readU32(input);
        header.versionMajor = readU32(input);
        header.versionMinor = readU32(input);
        header.fileCount = readU32(input);
        header.indexOffset = readU64(input);

        if (header.magic != GFS_MAGIC || header.versionMajor != GFS_VERSION_MAJOR) {
            GFS_DEBUG("Invalid header, please check file format and data integrity");
            return GfsResult::INVALID_DATA_ERROR;
        }

        GFS_DEBUG("--- Header informations ---");
        GFS_DEBUG("Magic: " << header.magic);
        GFS_DEBUG("Version: " << header.versionMajor << "." << header.versionMinor);
        GFS_DEBUG("File count: " << header.fileCount);
        GFS_DEBUG("Index pos: " << header.indexOffset);

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::readIndex(uint64_t indexPos, uint32_t fileCount, std::ifstream& input, Index &index)
    {
        input.seekg(indexPos, std::ios::beg);
        if (!input.good())
            return GfsResult::INVALID_DATA_ERROR;

        std::vector<char> buffer;
        for (uint32_t i = 0; i < fileCount; i++)
        {
            IndexEntry entry;
            entry.size = readU64(input);
            entry.offset = readU64(input);

            uint32_t pathLength = readU32(input);
            // Safety net to avoid large memory allocation in case of reading failure
            if (!input.good() || pathLength > 4096)
                return GfsResult::INVALID_DATA_ERROR;

            buffer.resize(pathLength);
            input.read(buffer.data(), buffer.size());
            entry.virtualPath = std::string(buffer.begin(), buffer.end());

            index.push_back(entry);

            GFS_DEBUG("-- Index entry --");
            GFS_DEBUG("Size: " << entry.size);
            GFS_DEBUG("Offset: " << entry.offset);
            GFS_DEBUG("Path length: " << pathLength);
            GFS_DEBUG("Path: " << entry.virtualPath << "\n");
        }

        return GfsResult::SUCCESS;
    }

    inline uint64_t internal::readU64(std::ifstream& input)
    {
        uint32_t high = readU32(input);
        uint32_t low = readU32(input);

        return (static_cast<uint64_t>(high) << 32) | static_cast<uint64_t>(low);
    }

    inline uint32_t internal::readU32(std::ifstream& input)
    {
        unsigned char bytes[4];
        input.read(reinterpret_cast<char*>(bytes), sizeof(bytes));

        return (static_cast<uint32_t>(bytes[0]) << 24) |
            (static_cast<uint32_t>(bytes[1]) << 16) |
            (static_cast<uint32_t>(bytes[2]) << 8) |
            (static_cast<uint32_t>(bytes[3]));
    }

    inline GfsResult internal::writeHeaderPlaceholder(std::ostream& output)
    {
        writeU32(output, GFS_MAGIC);
        writeU32(output, GFS_VERSION_MAJOR);
        writeU32(output, GFS_VERSION_MINOR);

        // These fields will be filled after content and index
        writeU32(output, UINT32_MAX); // File count
        writeU64(output, UINT64_MAX); // Index offset

        if (!output.good())
            return GfsResult::WRITE_ERROR;

        return GfsResult::SUCCESS;
    }

    // The format uses Big endian
    inline void internal::writeU32(std::ostream& output, uint32_t value)
    {
        unsigned char bytes[4];
        bytes[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
        bytes[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
        bytes[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
        bytes[3] = static_cast<unsigned char>(value & 0xFF);
        output.write(reinterpret_cast<char*>(bytes), sizeof(bytes));
    }

    inline void internal::writeU64(std::ostream& output, uint64_t value)
    {
        writeU32(output, static_cast<uint32_t>((value >> 32) & 0xFFFFFFFFu));
        writeU32(output, static_cast<uint32_t>(value & 0xFFFFFFFFu));
    }

    inline GfsResult internal::writeIndex(const Index& index, std::ostream& output)
    {
        for (auto& entry : index)
        {
            writeU64(output, entry.size);
            writeU64(output, entry.offset);

            uint32_t pathLength = static_cast<uint32_t>(entry.virtualPath.size());
            writeU32(output, pathLength);

            output.write(entry.virtualPath.data(), pathLength);

            if (!output.good())
                return GfsResult::WRITE_ERROR;
        }

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::writeContent(const std::vector<SourceFile>& files, std::ostream& output, std::vector<IndexEntry>& index)
    {
        for (auto& file : files)
        {
            std::ifstream source(file.sourcePath.c_str(), std::ifstream::binary);

            if (!source || !source.is_open())
            {
                GFS_DEBUG("Error, could not open file " << file.sourcePath);
                return GfsResult::SOURCE_FILE_ERROR;
            }
            IndexEntry entry;
            entry.offset = static_cast<uint64_t>(output.tellp());
            entry.virtualPath = file.virtualPath;

            std::vector<char> buffer(4096);
            while (source)
            {
                source.read(buffer.data(), buffer.size());
                std::streamsize readBytes = source.gcount();

                if (readBytes <= 0)
                    break;

                entry.size += static_cast<uint64_t>(readBytes);
                output.write(buffer.data(), readBytes);
            }

            if (!output.good())
                return GfsResult::WRITE_ERROR;

            index.push_back(entry);
        }

        return GfsResult::SUCCESS;
    }
}

#endif