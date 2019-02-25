#ifndef H_TYPES
#define H_TYPES

#include <map>
#include <memory>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> stringmap;
typedef std::vector<uint8_t> bytes_t;

template <typename T> using sptr = std::shared_ptr<T>;
template <typename T> using uptr = std::unique_ptr<T>;
using std::string;

#endif