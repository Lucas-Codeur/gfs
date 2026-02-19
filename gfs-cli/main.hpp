#pragma once;

#include <string>
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <gfs.hpp>

void help_command(int argc, char **argv);
void version_command(int argc, char **argv);
void info_command(int argc, char **argv);
void list_command(int argc, char **argv);
void create_command(int argc, char **argv);
void extract_command(int argc, char **argv);
void extract_all_command(int argc, char **argv);

namespace fs = std::filesystem;

struct CommandDesc
{
    std::string description;
    std::string usage;
    void (*fptr)(int, char **);
};

const std::unordered_map<std::string, CommandDesc> commands = {
    {"help", {"Shows this message", "help <command>", help_command}},
    {"version", {"Shows GFS (and CLI) version", "version", version_command}},
    {"info", {"Displays archive infos", "info -i <file_file>", info_command}},
    {"list", {"Prints archive index", "list -i <file_file>", list_command}},
    {"create", {"Creates an archive from the input directory", "create -i <root_dir> -o <output_archive>", create_command}},
    {"extract", {"Extracts a file from an archive", "extract -i <archive_file> -f <virtual_path> -o <output_file>", extract_command}},
    {"extractall", {"Extracts all files from an archive", "extractall -i <archive_file> -o <output_dir>", extract_all_command}},
};

static const std::string &BAD_ARGUMENTS = "Invalid arguments, try running: help <command>";

std::optional<fs::path> resolveInputFile(const char *strPath);
std::optional<fs::path> resolveOutputFile(const char *strPath);

const char *getArgument(const std::string &key, const int argc, char **argv);
const CommandDesc *getCommand(const std::string &name);

bool ask_proceed(const std::string &prompt);