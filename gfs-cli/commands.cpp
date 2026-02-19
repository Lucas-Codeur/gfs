#include "main.hpp"

void help_command(int argc, char **argv)
{
    // Display command list
    if (argc <= 2)
    {
        size_t longestCommandLength = 0;

        for (auto &[command, descriptor] : commands)
        {
            if (command.size() > longestCommandLength)
                longestCommandLength = command.size();
        }

        std::cout << "usage: gfs-cli <command> [<args>]\n\n"
                  << "Here the list of GFS cli commands:"
                  << std::endl;

        for (auto &[command, descriptor] : commands)
        {
            std::cout << "    " << command << "      " << std::string(longestCommandLength - command.size(), ' ') << descriptor.description << std::endl;
        }
    }
    // Display specific command info
    else
    {
        const CommandDesc *matching = getCommand(std::string(argv[2]));

        if (!matching)
        {
            std::cout << "Command '" << argv[2] << "' not found";
            return;
        }

        std::cout << argv[2] << " - " << matching->description << " - usage: " << matching->usage << std::endl;
    }
}

void version_command(int argc, char **argv)
{
    std::cout << "GFS cli for GFS 1.0.0" << std::endl;
}

void info_command(int argc, char **argv)
{
    auto inputPath = resolveInputFile(getArgument("i", argc, argv));
    if (!inputPath)
        return;

    gfs::Archive archive;
    auto rs = gfs::readArchive(inputPath->generic_string(), archive);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not open or read archive. Code : " << rs << std::endl;
        return;
    }

    std::cout << "--- Archive informations: " << inputPath->filename() << " ---" << "\n";
    std::cout << "Format:       " << "GFS" << "\n";
    std::cout << "Version:      " << archive.header.versionMajor << "." << archive.header.versionMinor << "\n";
    std::cout << "Size:         " << fs::file_size(*inputPath) << " b" << "\n";
    std::cout << "File count:   " << archive.header.fileCount << "\n";
    std::cout << "Index offset: " << archive.header.indexOffset << std::endl;
}

void list_command(int argc, char **argv)
{
    auto inputPath = resolveInputFile(getArgument("i", argc, argv));
    if (!inputPath)
        return;

    gfs::Index index;
    auto rs = gfs::listFiles(inputPath->generic_string(), index);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not open or read archive. Code : " << rs << std::endl;
        return;
    }

    std::cout << "--- Archive index: " << inputPath->filename() << " ---" << "\n";

    for (auto &[virtualPath, entry] : index)
    {
        std::cout << "\n";
        std::cout << "Path:        " << virtualPath << "\n";
        std::cout << "Path length: " << virtualPath.size() << "\n";
        std::cout << "Size:        " << entry.size << "\n";
        std::cout << "Offset:      " << entry.offset << std::endl;
    }
}

void create_command(int argc, char **argv)
{
    const char *rootPath = getArgument("i", argc, argv);
    if (rootPath == nullptr)
    {
        std::cout << "Please specify root directory" << std::endl;
        return;
    }

    fs::path fsRootDirPath(rootPath);
    if (!fs::exists(fsRootDirPath) || !fs::is_directory(fsRootDirPath))
    {
        std::cout << "Invalid root directory: " << fs::absolute(fsRootDirPath) << std::endl;
        ;
        return;
    }

    auto outputPath = resolveOutputFile(getArgument("o", argc, argv));
    if (!outputPath)
        return;

    std::vector<gfs::SourceFile> sourceFiles;

    fs::recursive_directory_iterator it(fsRootDirPath, fs::directory_options::skip_permission_denied);
    for (const auto &entry : it)
    {
        if (!entry.is_regular_file())
            continue;

        const std::string filePath = fs::absolute(entry.path()).generic_string();
        const std::string relativePath = entry.path().lexically_relative(fsRootDirPath).generic_string();

        sourceFiles.push_back({filePath, relativePath});
    }

    auto rs = gfs::createArchive(fs::absolute(*outputPath).generic_string(), sourceFiles);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not create archive. Code : " << rs << std::endl;
        return;
    }
}

void extract_command(int argc, char **argv)
{
    const char *virtualPath = getArgument("f", argc, argv);
    if (virtualPath == nullptr)
    {
        std::cout << "Please specify virtual path" << std::endl;
        return;
    }

    auto outputPath = resolveOutputFile(getArgument("o", argc, argv));
    if (!outputPath)
        return;

    auto inputPath = resolveInputFile(getArgument("i", argc, argv));
    if (!inputPath)
        return;

    gfs::Archive archive;
    auto rs = gfs::readArchive(inputPath->generic_string(), archive);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not open or read archive. Code : " << rs << std::endl;
        return;
    }

    std::ifstream archiveStream;
    rs = gfs::openArchiveInputStream(archive, archiveStream);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not open archive stream. Code : " << rs << std::endl;
        return;
    }

    rs = gfs::extractFile(virtualPath, archive, archiveStream, fs::absolute(*outputPath).generic_string());
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not extract file. Code : " << rs << std::endl;
        return;
    }

    std::cout << "File extracted successfully." << std::endl;
}

void extract_all_command(int argc, char **argv)
{
    auto inputPath = resolveInputFile(getArgument("i", argc, argv));
    if (!inputPath)
        return;

    const char *strPath = getArgument("o", argc, argv);
    if (strPath == nullptr)
    {
        std::cout << "Please specify output directory" << std::endl;
        return;
    }

    fs::path outDirPath(strPath);
    outDirPath = fs::absolute(outDirPath);
    if (!fs::exists(outDirPath))
    {
        std::cout << "Invalid output directory: " << fs::absolute(outDirPath) << std::endl;
        return;
    }

    if (!fs::is_empty(outDirPath))
    {
        if (!ask_proceed("The output dir is not empty, hence files with matching paths will be overwritten, do you wish to proceed ?"))
            return;
    }

    gfs::Archive archive;
    auto rs = gfs::readArchive(inputPath->generic_string(), archive);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not open or read archive. Code : " << rs << std::endl;
        return;
    }

    std::ifstream archiveStream;
    rs = gfs::openArchiveInputStream(archive, archiveStream);
    if (rs != gfs::GfsResult::SUCCESS)
    {
        std::cout << "Could not open archive stream. Code : " << rs << std::endl;
        return;
    }

    for (auto &[virtualPath, entry] : archive.index)
    {
        std::filesystem::path fileOutput = std::filesystem::absolute(outDirPath / fs::path(virtualPath));
        if (!fs::exists(fileOutput.parent_path()))
            fs::create_directories(fileOutput.parent_path());

        rs = gfs::extractFile(virtualPath, archive, archiveStream, fs::absolute(fileOutput).generic_string());
        if (rs != gfs::GfsResult::SUCCESS)
        {
            std::cout << "Could not extract file : " << virtualPath << " to " << fileOutput << ". Code : " << rs << std::endl;
            return;
        }
    }

    std::cout << "Archive extracted successfully." << std::endl;
}