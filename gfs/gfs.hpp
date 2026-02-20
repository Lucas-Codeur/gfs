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
#include <unordered_map>
#include <algorithm>

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
    inline constexpr uint32_t GFS_VERSION_MINOR = 2;
    inline constexpr uint32_t GFS_VERSION_PATCH = 0; // Used for Cmake version

    inline constexpr uint32_t GFS_MAGIC = 0x47465330; // GFS0
    inline constexpr uint32_t GFS_MAX_PATH_SIZE = 2048;

    inline constexpr unsigned int GFS_HEADER_SIZE = 4 * sizeof(uint32_t) + 1 * sizeof(uint64_t);

    enum class GfsResult
    {
        SUCCESS = 0,
        OUTPUT_FILE_ERROR = 1,
        SOURCE_FILE_ERROR = 2,
        WRITE_ERROR = 3,
        INPUT_FILE_ERROR = 4,
        INVALID_DATA_ERROR = 5,
        ARCHIVE_NOT_LOADED_ERROR = 6,
        ARCHIVE_FILE_NOT_FOUND_ERROR = 7,
        DUPLICATE_FILE_ERROR = 8
    };
    // Allows easy result printing
    inline std::ostream &operator<<(std::ostream &output, gfs::GfsResult result);

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
    };

    using Index = std::unordered_map<std::string, IndexEntry>;

    struct Archive
    {
        std::string path;
        Header header;
        Index index; // Maps virtual path to index entry
        bool loaded = false;
    };

    struct SourceFile
    {
        std::string sourcePath;
        std::string virtualPath;
    };

    inline GfsResult createArchive(const std::string &outputPath, const std::vector<SourceFile> &files);
    inline GfsResult readArchive(const std::string &path, Archive &archive);
    inline GfsResult openArchiveInputStream(const Archive &archive, std::ifstream &stream);
    inline GfsResult readArchivedFile(const std::string &virtualPath, std::ifstream &stream, const Archive &archive, std::vector<char> &data);

    inline GfsResult extractFile(const std::string &virtualPath, const Archive &archive, std::ifstream &archiveInput, const std::string &outputPath);
    inline GfsResult listFiles(const std::string &path, Index &index);

    namespace internal
    {
        // Write related stuff
        inline GfsResult writeHeaderPlaceholder(std::ostream &output);
        inline GfsResult writeContent(const std::vector<SourceFile> &files, std::ostream &output, Index &index);
        inline GfsResult writeIndex(const Index &index, std::ostream &output);

        inline void writeU32(std::ostream &output, uint32_t value);
        inline void writeU64(std::ostream &output, uint64_t value);

        // Read related stuff
        inline GfsResult readHeader(std::ifstream &input, Header &header);
        inline GfsResult readIndex(uint64_t indexPos, uint32_t fileCount, std::ifstream &input, Index &index);

        inline uint32_t readU32(std::ifstream &input);
        inline uint64_t readU64(std::ifstream &input);

        inline GfsResult openFileInputStream(const std::string &path, std::ifstream &stream);
        inline GfsResult openFileOutputStream(const std::string &path, std::ofstream &stream);
    }
}

// Implementation
namespace gfs
{
    inline GfsResult createArchive(const std::string &outputPath, const std::vector<SourceFile> &files)
    {
        GFS_DEBUG("Attempting to create archive " << outputPath);

        std::ofstream file;
        GfsResult stepResult = internal::openFileOutputStream(outputPath, file);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        internal::writeHeaderPlaceholder(file);

        Index index;
        stepResult = internal::writeContent(files, file, index);
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

    inline GfsResult readArchive(const std::string &path, Archive &archive)
    {
        std::ifstream file;
        GfsResult stepResult = internal::openFileInputStream(path.c_str(), file);

        if (stepResult != GfsResult::SUCCESS)
        {
            GFS_DEBUG("Error, could not open archive " << path);
            return stepResult;
        }

        Header header;
        stepResult = internal::readHeader(file, header);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        stepResult = internal::readIndex(header.indexOffset, header.fileCount, file, archive.index);
        if (stepResult != GfsResult::SUCCESS)
            return stepResult;

        archive.header = header;
        archive.path = path;
        archive.loaded = true;
        return GfsResult::SUCCESS;
    }

    inline GfsResult listFiles(const std::string &path, Index &index)
    {
        Archive archive;
        GfsResult result = readArchive(path, archive);

        if (result != GfsResult::SUCCESS)
            return result;

        index = std::move(archive.index);
        return result;
    }

    // Warning : the output file will be overwritten
    inline GfsResult extractFile(const std::string &virtualPath, const Archive &archive, std::ifstream &archiveInput, const std::string &outputPath)
    {
        if (!archive.loaded)
            return GfsResult::ARCHIVE_NOT_LOADED_ERROR;

        auto it = archive.index.find(virtualPath);
        if (it == archive.index.end())
            return GfsResult::ARCHIVE_FILE_NOT_FOUND_ERROR;

        const IndexEntry &matching = it->second;

        std::ofstream output;
        GfsResult result = internal::openFileOutputStream(outputPath, output);
        if (result != GfsResult::SUCCESS)
            return result;

        std::vector<char> buffer(4096);

        uint64_t currentOffset = matching.offset;
        uint64_t end = matching.offset + matching.size;

        archiveInput.seekg(currentOffset, std::ios::beg);
        if (!archiveInput.good())
            return GfsResult::INVALID_DATA_ERROR;

        while (archiveInput)
        {
            if (currentOffset >= end)
                break;

            archiveInput.read(buffer.data(), static_cast<std::streamsize>(std::min(buffer.size(), end - currentOffset)));
            std::streamsize readBytes = archiveInput.gcount();

            if (readBytes <= 0)
                break;

            currentOffset += static_cast<uint64_t>(readBytes);
            output.write(buffer.data(), readBytes);
        }

        if (!output.good())
            return GfsResult::WRITE_ERROR;

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::openFileOutputStream(const std::string &path, std::ofstream &stream)
    {
        stream.open(path, std::ios::binary);
        if (!stream || !stream.is_open())
            return GfsResult::OUTPUT_FILE_ERROR;
        return GfsResult::SUCCESS;
    }

    inline GfsResult openArchiveInputStream(const Archive &archive, std::ifstream &stream)
    {
        if (!archive.loaded)
            return GfsResult::ARCHIVE_NOT_LOADED_ERROR;
        return internal::openFileInputStream(archive.path, stream);
    }

    inline GfsResult internal::openFileInputStream(const std::string &path, std::ifstream &stream)
    {
        stream.open(path, std::ifstream::binary);
        if (!stream || !stream.is_open())
            return GfsResult::INPUT_FILE_ERROR;
        return GfsResult::SUCCESS;
    }

    inline GfsResult readArchivedFile(const std::string &virtualPath, std::ifstream &stream, const Archive &archive, std::vector<char> &data)
    {
        if (!archive.loaded)
            return GfsResult::ARCHIVE_NOT_LOADED_ERROR;

        auto it = archive.index.find(virtualPath);
        if (it == archive.index.end())
            return GfsResult::ARCHIVE_FILE_NOT_FOUND_ERROR;

        const IndexEntry &matching = it->second;

        stream.seekg(matching.offset, std::ios::beg);
        if (!stream.good())
            return GfsResult::INVALID_DATA_ERROR;

        data.resize(matching.size);
        stream.read(data.data(), matching.size);

        // If case there is some missing data
        if (stream.gcount() != static_cast<std::streamsize>(matching.size))
            return GfsResult::INVALID_DATA_ERROR;

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::readHeader(std::ifstream &input, Header &header)
    {
        header.magic = readU32(input);
        header.versionMajor = readU32(input);
        header.versionMinor = readU32(input);
        header.fileCount = readU32(input);
        header.indexOffset = readU64(input);

        if (header.magic != GFS_MAGIC || header.versionMajor != GFS_VERSION_MAJOR)
        {
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

    inline GfsResult internal::readIndex(uint64_t indexPos, uint32_t fileCount, std::ifstream &input, Index &index)
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
            if (!input.good() || pathLength > GFS_MAX_PATH_SIZE)
                return GfsResult::INVALID_DATA_ERROR;

            buffer.resize(pathLength);
            input.read(buffer.data(), buffer.size());

            std::string virtualPath = std::string(buffer.begin(), buffer.end());
            index[virtualPath] = entry;

            GFS_DEBUG("-- Index entry --");
            GFS_DEBUG("Size: " << entry.size);
            GFS_DEBUG("Offset: " << entry.offset);
            GFS_DEBUG("Path length: " << pathLength);
            GFS_DEBUG("Path: " << virtualPath << "\n");
        }

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::writeHeaderPlaceholder(std::ostream &output)
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

    inline GfsResult internal::writeIndex(const Index &index, std::ostream &output)
    {
        for (auto &[path, entry] : index)
        {
            writeU64(output, entry.size);
            writeU64(output, entry.offset);

            uint32_t pathLength = static_cast<uint32_t>(path.size());
            writeU32(output, pathLength);

            output.write(path.data(), pathLength);

            if (!output.good())
                return GfsResult::WRITE_ERROR;
        }

        return GfsResult::SUCCESS;
    }

    inline GfsResult internal::writeContent(const std::vector<SourceFile> &files, std::ostream &output, Index &index)
    {
        for (auto &file : files)
        {
            if (index.count(file.virtualPath) > 0)
                return GfsResult::INVALID_DATA_ERROR;

            std::ifstream source;
            GfsResult result = internal::openFileInputStream(file.sourcePath.c_str(), source);

            if (result != GfsResult::SUCCESS)
            {
                GFS_DEBUG("Error, could not open file " << file.sourcePath);
                return GfsResult::SOURCE_FILE_ERROR;
            }
            IndexEntry entry;
            entry.offset = static_cast<uint64_t>(output.tellp());

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

            index[file.virtualPath] = entry;
        }

        return GfsResult::SUCCESS;
    }

    inline uint64_t internal::readU64(std::ifstream &input)
    {
        // These two instruction need to stay in this order
        uint32_t low = readU32(input);
        uint32_t high = readU32(input);

        return (static_cast<uint64_t>(high) << 32) | static_cast<uint64_t>(low);
    }

    inline uint32_t internal::readU32(std::ifstream &input)
    {
        unsigned char bytes[4];
        input.read(reinterpret_cast<char *>(bytes), sizeof(bytes));

        return (static_cast<uint32_t>(bytes[3]) << 24) |
               (static_cast<uint32_t>(bytes[2]) << 16) |
               (static_cast<uint32_t>(bytes[1]) << 8) |
               (static_cast<uint32_t>(bytes[0]));
    }

    inline void internal::writeU32(std::ostream &output, uint32_t value)
    {
        unsigned char bytes[4];
        bytes[0] = static_cast<unsigned char>(value & 0xFF);
        bytes[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
        bytes[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
        bytes[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
        output.write(reinterpret_cast<char *>(bytes), sizeof(bytes));
    }

    inline void internal::writeU64(std::ostream &output, uint64_t value)
    {
        writeU32(output, static_cast<uint32_t>(value & 0xFFFFFFFFu));
        writeU32(output, static_cast<uint32_t>((value >> 32) & 0xFFFFFFFFu));
    }

    inline std::ostream &operator<<(std::ostream &output, gfs::GfsResult result)
    {
        switch (result)
        {
        case gfs::GfsResult::SUCCESS:
            return output << "SUCCESS";
        case gfs::GfsResult::OUTPUT_FILE_ERROR:
            return output << "OUTPUT_FILE_ERROR";
        case gfs::GfsResult::SOURCE_FILE_ERROR:
            return output << "SOURCE_FILE_ERROR";
        case gfs::GfsResult::WRITE_ERROR:
            return output << "WRITE_ERROR";
        case gfs::GfsResult::INPUT_FILE_ERROR:
            return output << "INPUT_FILE_ERROR";
        case gfs::GfsResult::INVALID_DATA_ERROR:
            return output << "INVALID_DATA_ERROR";
        case gfs::GfsResult::ARCHIVE_NOT_LOADED_ERROR:
            return output << "ARCHIVE_NOT_LOADED_ERROR";
        case gfs::GfsResult::ARCHIVE_FILE_NOT_FOUND_ERROR:
            return output << "ARCHIVE_FILE_NOT_FOUND_ERROR";
        case gfs::GfsResult::DUPLICATE_FILE_ERROR:
            return output << "DUPLICATE_FILE_ERROR";
        default:
            // If you ever get this code error while using gfs functions, please report on github
            return output << "UNKNOWN";
        }
    };
}

#endif