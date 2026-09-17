#ifndef BRICKRED_STRING_UTIL_H
#define BRICKRED_STRING_UTIL_H

#include <cstddef>
#include <string>
#include <vector>

namespace brickred::string_util {

void split(const char *str, const char *sep,
           std::vector<std::string> *result, int max_split = -1);
void split(const char *str, size_t str_len, const char *sep,
           std::vector<std::string> *result, int max_split = -1);

std::string ltrim(const std::string &str, const char *ws = " \t");
std::string rtrim(const std::string &str, const char *ws = " \t");
std::string trim(const std::string &str, const char *ws = " \t");
std::string toUpper(const std::string &str);
std::string toLower(const std::string &str);
std::string replace(const std::string &str,
                    const std::string &search,
                    const std::string &replace,
                    int max_replace = -1);

std::string toString(int v);
std::string toString(long v);
std::string toString(long long v);
std::string toString(unsigned v);
std::string toString(unsigned long v);
std::string toString(unsigned long long v);

bool strictFromString(const char *str, size_t str_len, short &v);
bool strictFromString(const char *str, size_t str_len, int &v);
bool strictFromString(const char *str, size_t str_len, long &v);
bool strictFromString(const char *str, size_t str_len, long long &v);
bool strictFromString(const char *str, size_t str_len, unsigned short &v);
bool strictFromString(const char *str, size_t str_len, unsigned &v);
bool strictFromString(const char *str, size_t str_len, unsigned long &v);
bool strictFromString(const char *str, size_t str_len, unsigned long long &v);
bool strictFromString(const std::string &str, short &v);
bool strictFromString(const std::string &str, int &v);
bool strictFromString(const std::string &str, long &v);
bool strictFromString(const std::string &str, long long &v);
bool strictFromString(const std::string &str, unsigned short &v);
bool strictFromString(const std::string &str, unsigned &v);
bool strictFromString(const std::string &str, unsigned long &v);
bool strictFromString(const std::string &str, unsigned long long &v);

const char *find(const char *str, size_t str_len, const char *keyword);
bool caseInsensitiveEqual(const std::string &lhs, const std::string &rhs);

struct Hash {
    size_t operator()(const std::string &str) const;
};

struct CaseInsensitiveLess {
    bool operator()(const std::string &lhs, const std::string &rhs) const;
};

} // namespace brickred::string_util

#endif
