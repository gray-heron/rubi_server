
#ifndef H_COMMONS
#define H_COMMONS

#include <map>
#include <memory>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> stringmap;
typedef std::vector<uint8_t> bytes_t;

template <typename T> using sptr = std::shared_ptr<T>;
template <typename T> using uptr = std::unique_ptr<T>;
using std::string;

bool parseArguments(std::string name_prefix, char name_value_sep, int argc,
                    char **argv, stringmap &out);

class Log
{
  public:
    static bool throw_on_warning;

    static void Info(std::string msg);
    static void Warning(std::string msg);
    static void Error(std::string msg);
};

#endif