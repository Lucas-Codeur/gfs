#include "main.hpp"

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        help_command(argc, argv);
        return 0;
    }

    const CommandDesc *matching = getCommand(std::string(argv[1]));

    if (matching)
    {
        matching->fptr(argc, argv);
    }
    else
    {
        help_command(argc, argv);
    }

    return 0;
}

const char *getArgument(const std::string &key, const int argc, char **argv)
{
    std::string flag = "-" + key;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == flag && argc > i + 1)
            return argv[i + 1];
    }

    return nullptr;
}

const CommandDesc *getCommand(const std::string &name)
{
    for (auto &[command, descriptor] : commands)
    {
        if (name == command)
        {
            return &descriptor;
        }
    }

    return nullptr;
}

std::optional<fs::path> resolveInputFile(const char *strPath)
{
    if (!strPath)
        return std::nullopt;
    fs::path fsPath(strPath);

    if (!fs::exists(fsPath))
    {
        std::cout << "File not found: " << fs::absolute(fsPath) << std::endl;
        return std::nullopt;
    }

    return fs::canonical(fsPath);
}

std::optional<fs::path> resolveOutputFile(const char *strPath)
{
    if (!strPath)
        return std::nullopt;
    fs::path fsPath(strPath);

    if (fs::is_directory(fsPath) || fs::exists(fsPath))
    {
        std::cout << "Invalid or existing output path: " << fs::absolute(fsPath) << std::endl;
        return std::nullopt;
    }

    return fs::absolute(fsPath);
}

bool ask_proceed(const std::string &prompt)
{
    // Repeat until correct input
    while (true)
    {
        std::cout << prompt << " [y/n]: ";
        std::string input;
        std::getline(std::cin, input);

        // To lowercase
        std::transform(input.begin(), input.end(), input.begin(),
                       [](unsigned char x)
                       { return std::tolower(x); });

        if (input == "y" || input == "yes")
        {
            return true;
        }
        if (input == "n" || input == "no")
        {
            return false;
        }
    }
}