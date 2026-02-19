#include <string>
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <gfs.hpp>

struct CommandDesc {
	std::string description;
	std::string usage;
	void (*fptr)(int, char**);
};

void help_command(int argc, char** argv);
void version_command(int argc, char** argv);
void info_command(int argc, char** argv);

const std::unordered_map<std::string, CommandDesc> commands = {
		{"help", {"Shows this message", "help <command>", help_command}},
		{"version", {"Shows GFS (and CLI) version", "version", version_command}},
		{"info", {"Displays archive data", "info -i <file>", info_command}}
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

	std::filesystem::path fsPath(inputPath);
	if (!std::filesystem::exists(fsPath)) {
		std::cout << "File not found: " << std::filesystem::absolute(fsPath) << std::endl;;
		return;
	}

	std::filesystem::path absolute = std::filesystem::canonical(fsPath);

	gfs::Archive archive;
	if (auto rs = gfs::readArchive(absolute.generic_string(), archive) != gfs::GfsResult::SUCCESS) {
		std::cout << "Could not open or read archive. Code : " << rs << std::endl;
		return;
	}

	std::cout << "--- Archive informations: " << absolute.filename() << " ---" << "\n";
	std::cout << "Format:       " << "GFS" << "\n";
	std::cout << "Version:      " << archive.header.versionMajor << "." << archive.header.versionMinor << "\n";
	std::cout << "Size:         " << std::filesystem::file_size(absolute) << " b" << "\n";
	std::cout << "File count:   " << archive.header.fileCount << "\n";
	std::cout << "Index offset: " << archive.header.indexOffset << std::endl;
}