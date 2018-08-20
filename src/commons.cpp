
#include "commons.h"

using std::string;

bool Log::throw_on_warning = false;
void Log::Info(string msg) { printf("[INFO]\t%s\n", msg.c_str()); }

void Log::Error(string msg)
{
    printf("[ERR]\t%s\n", msg.c_str());
    throw std::exception();
}

void Log::Warning(string msg)
{
    printf("[WARN]\t%s\n", msg.c_str());
    if (throw_on_warning)
        Error("Error on warning!");
}

bool parseArguments(string name_prefix, char name_value_sep, int argc,
                    char **argv, stringmap &out)
{
    bool clean = true;

    for (int arg_i = 1; arg_i < argc; arg_i++)
    {
        string current_argument(argv[arg_i]), current_name, current_value;
        int cursor = name_prefix.length();

        if (name_prefix != "" && current_argument.find(name_prefix) != 0)
        {
            Log::Warning((string) "Wrong prefix on argument: " +
                         current_argument + "!");
            clean = false;
            continue;
        }

        while (cursor < current_argument.length() &&
               current_argument[cursor] != name_value_sep)
        {
            current_name += current_argument[cursor];
            cursor++;
        }

        if (cursor == current_argument.length())
        {
            Log::Warning((string) "No separator on argument: " +
                         current_argument + "!");
            clean = false;
            continue;
        }

        if (out.find(current_name) != out.end())
        {
            Log::Warning((string) "Argument repetition: " + current_name + "!");
            clean = false;
        }

        for (cursor += 1; cursor < current_argument.length(); cursor++)
            current_value += current_argument[cursor];

        out[current_name] = current_value;
    }

    return clean;
}