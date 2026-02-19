#include <string>
#include <unordered_map>
#include <iostream>
#include <gfs.hpp>

struct CommandDesc {
	std::string help;
	void (*fptr)(int, char**);
};

void help_command(int argc, char** argv);

//Helper function
const char* getArgument(const std::string& key, const int argc, const char** argv) {
	std::string flag = "--" + key;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == flag && argc > i + 1)
			return argv[i + 1];
	}

	return nullptr;
}

const std::unordered_map<std::string, CommandDesc> commands = {
		{"help", {"Shows this message", help_command}}
};

int main(int argc, char** argv) {
	if (argc <= 1) {
		std::cout << "usage: gfs-cli <command> [<args>]" << std::endl;
		return 0;
	}

	if (strcmp(argv[1], "version") == 0) {
		std::cout << "GFS cli for GFS 1.0.0" << std::endl;
	}

	const CommandDesc* matching = nullptr;
	for (auto& [command, descriptor] : commands) {
		if (strcmp(argv[1], command.c_str()) == 0) {
			matching = &descriptor;
			break;
		}
	}
	
	if (matching) {
		matching->fptr(argc, argv);
	} else {
		help_command(argc, argv);
	}
	
	return 0;
}

void help_command(int argc, char** argv) {
	std::cout << "usage: gfs-cli <command> [<args>]\n\n"
		<< "Here the list of GFS cli commands:"
		<< std::endl;

	for (auto& [command, descriptor] : commands) {
		std::cout << "    " << command << "        " << descriptor.help << std::endl;
	}
}