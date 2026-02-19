#include <string>
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <gfs.hpp>

namespace fs = std::filesystem;

struct CommandDesc {
	std::string description;
	std::string usage;
	void (*fptr)(int, char**);
};

void help_command(int argc, char** argv);
void version_command(int argc, char** argv);
void info_command(int argc, char** argv);
void list_command(int argc, char** argv);
void create_command(int argc, char** argv);
void extract_command(int argc, char** argv);

const std::unordered_map<std::string, CommandDesc> commands = {
		{"help",    {"Shows this message", "help <command>", help_command}},
		{"version", {"Shows GFS (and CLI) version", "version", version_command}},
		{"info",    {"Displays archive infos", "info -i <file_file>", info_command}},
		{"list",    {"Prints archive index", "list -i <file_file>", list_command}},
		{"create",  {"Creates an archive from the input directory", "create -i <root_dir> -o <output_archive>", create_command}},
		{"extract", {"Extracts a file from an archive", "create -i <archive_file> -f <virtual_path> -o <output_file>", extract_command}},
};

static const std::string BAD_ARGUMENTS = "Invalid arguments, try running: help <command>";

//Helper function
const char* getArgument(const std::string& key, const int argc, char** argv) {
	std::string flag = "-" + key;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == flag && argc > i + 1)
			return argv[i + 1];
	}

	return nullptr;
}

const CommandDesc* getCommand(const std::string& name) {
	for (auto& [command, descriptor] : commands) {
		if (name == command) {
			return &descriptor;
		}
	}

	return nullptr;
}

int main(int argc, char** argv) {
	if (argc <= 1) {
		help_command(argc, argv);
		return 0;
	}

	const CommandDesc* matching = getCommand(std::string(argv[1]));
	
	if (matching) {
		matching->fptr(argc, argv);
	} else {
		help_command(argc, argv);
	}
	
	return 0;
}

void help_command(int argc, char** argv) {
	// Display command list
	if (argc <= 2) {
		size_t longestCommandLength = 0;

		for (auto& [command, descriptor] : commands) {
			if (command.size() > longestCommandLength)
				longestCommandLength = command.size();
		}

		std::cout << "usage: gfs-cli <command> [<args>]\n\n"
			<< "Here the list of GFS cli commands:"
			<< std::endl;

		for (auto& [command, descriptor] : commands) {
			std::cout << "    " << command << "      " << std::string(longestCommandLength - command.size(), ' ') << descriptor.description << std::endl;
		}
	}
	// Display specific command info
	else {
		const CommandDesc* matching = getCommand(std::string(argv[2]));

		if (!matching) {
			std::cout << "Command '" << argv[2] << "' not found";
			return;
		}

		std::cout << argv[2] << " - " << matching->description << " - usage: " << matching->usage << std::endl;
	}
}

void version_command(int argc, char** argv) {
	std::cout << "GFS cli for GFS 1.0.0" << std::endl;
}

void info_command(int argc, char** argv) {
	const char* inputPath = getArgument("i", argc, argv);
	if (inputPath == nullptr) {
		std::cout << BAD_ARGUMENTS << std::endl;
		return;
	}

	fs::path fsPath(inputPath);
	if (!fs::exists(fsPath)) {
		std::cout << "File not found: " << fs::absolute(fsPath) << std::endl;;
		return;
	}

	fs::path absolute = fs::canonical(fsPath);

	gfs::Archive archive;
	auto rs = gfs::readArchive(absolute.generic_string(), archive);
	if (rs != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not open or read archive. Code : " << rs << std::endl;
		return;
	}

	std::cout << "--- Archive informations: " << absolute.filename() << " ---" << "\n";
	std::cout << "Format:       " << "GFS" << "\n";
	std::cout << "Version:      " << archive.header.versionMajor << "." << archive.header.versionMinor << "\n";
	std::cout << "Size:         " << fs::file_size(absolute) << " b" << "\n";
	std::cout << "File count:   " << archive.header.fileCount << "\n";
	std::cout << "Index offset: " << archive.header.indexOffset << std::endl;
}

void list_command(int argc, char** argv) {
	const char* inputPath = getArgument("i", argc, argv);
	if (inputPath == nullptr) {
		std::cout << BAD_ARGUMENTS << std::endl;
		return;
	}

	fs::path fsPath(inputPath);
	if (!fs::exists(fsPath)) {
		std::cout << "File not found: " << fs::absolute(fsPath) << std::endl;;
		return;
	}

	fs::path absolute = fs::canonical(fsPath);

	gfs::Index index;
	auto rs = gfs::listFiles(absolute.generic_string(), index);
	if (rs != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not open or read archive. Code : " << rs << std::endl;
		return;
	}

	std::cout << "--- Archive index: " << absolute.filename() << " ---" << "\n";

	for (auto &[virtualPath, entry] : index) {
		std::cout << "\n";
		std::cout << "Path:        " << virtualPath << "\n";
		std::cout << "Path length: " << virtualPath.size() << "\n";
		std::cout << "Size:        " << entry.size << "\n";
		std::cout << "Offset:      " << entry.offset << std::endl;
	}
}

void create_command(int argc, char** argv) {
    const char* rootPath = getArgument("i", argc, argv);
	if (rootPath == nullptr) {
		std::cout << "Please specify root directory" << std::endl;
		return;
	}

	fs::path fsRootDirPath(rootPath);
	if (!fs::exists(fsRootDirPath) || !fs::is_directory(fsRootDirPath)) {
		std::cout << "Invalid root directory: " << fs::absolute(fsRootDirPath) << std::endl;;
		return;
	}

    const char* outputPath = getArgument("o", argc, argv);
    if (outputPath == nullptr) {
		std::cout << "Please specify output file" << std::endl;
		return;
	}

    fs::path fsOutputPath(outputPath);
	if (fs::is_directory(fsOutputPath)) {
		std::cout << "Invalid output file: " << fs::absolute(fsOutputPath) << std::endl;;
		return;
	}

	if (fs::exists(fsOutputPath)) {
		std::cout << "File already exists: " << fs::absolute(fsOutputPath) << std::endl;;
		return;
	}

	std::vector<gfs::SourceFile> sourceFiles;

	fs::recursive_directory_iterator it(fsRootDirPath, fs::directory_options::skip_permission_denied);
	for (const auto& entry : it) {
		if (!entry.is_regular_file()) continue;

		const std::string filePath = fs::absolute(entry.path()).generic_string();
		const std::string relativePath = entry.path().lexically_relative(fsRootDirPath).generic_string();

		sourceFiles.push_back({ filePath, relativePath});
		std::cout << entry.path().lexically_relative(fsRootDirPath).generic_string() << '\n';
	}

	auto rs = gfs::createArchive(fs::absolute(fsOutputPath).generic_string(), sourceFiles);
	if (rs != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not create archive. Code : " << rs << std::endl;
		return;
	}
}

void extract_command(int argc, char** argv) {
	const char* virtualPath = getArgument("f", argc, argv);
	if (virtualPath == nullptr) {
		std::cout << "Please specify virtual path" << std::endl;
		return;
	}

	const char* outputPath = getArgument("o", argc, argv);
	if (outputPath == nullptr) {
		std::cout << "Please specify output file" << std::endl;
		return;
	}

	fs::path fsOutputPath(outputPath);
	if (fs::exists(fsOutputPath.parent_path()) || fs::is_directory(fsOutputPath)) {
		std::cout << "Invalid destination: " << fs::absolute(fsOutputPath) << std::endl;;
		return;
	}

	if (fs::exists(fsOutputPath)) {
		std::cout << "File already exists: " << fs::absolute(fsOutputPath) << std::endl;;
		return;
	}

	const char* inputPath = getArgument("i", argc, argv);
	if (inputPath == nullptr) {
		std::cout << "Please specify input file" << std::endl;
		return;
	}

	fs::path fsInputPath(inputPath);
	if (!fs::exists(fsInputPath)) {
		std::cout << "File not found: " << fs::absolute(fsInputPath) << std::endl;;
		return;
	}

	fs::path absoluteArchivePath = fs::canonical(fsInputPath);

	gfs::Archive archive;
	auto rs = gfs::readArchive(absoluteArchivePath.generic_string(), archive);
	if (rs != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not open or read archive. Code : " << rs << std::endl;
		return;
	}

	std::ifstream archiveStream;
	rs = gfs::openArchiveInputStream(archive, archiveStream);
	if (rs != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not open archive stream. Code : " << rs << std::endl;
		return;
	}

	rs = gfs::extractFile(virtualPath, archive, archiveStream, fs::absolute(fsOutputPath).generic_string());
	if (rs != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not extract file. Code : " << rs << std::endl;
		return;
	}

	std::cout << "File extracted successfully." << std::endl;
}