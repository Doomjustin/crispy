#ifndef CRISPY_REDIS_MESSAGE_H
#define CRISPY_REDIS_MESSAGE_H

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace crispy::redis {

constexpr const char* NOT_AN_INTEGER_OR_OUT_OF_RANGE =
    "-ERR value is not an integer or out of range\r\n";

constexpr const char* WRONG_TYPE_OPERATION =
    "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";

constexpr const char* SYNTAX_ERROR = "-ERR syntax error\r\n";

constexpr const char* INVALID_DB_INDEX = "-ERR invalid DB index\r\n";

constexpr const char* OK = "+OK\r\n";

constexpr const char* NIL = "$-1\r\n";

namespace resp {

inline auto unknown_command(std::string_view cmd) -> std::string
{
    return std::format("-ERR unknown command '{}'\r\n", cmd);
}

inline auto wrong_number_of_arguments(std::string_view cmd) -> std::string
{
    return std::format("-ERR wrong number of arguments for '{}' command\r\n", cmd);
}

inline auto bulk_string(std::string_view str) -> std::string
{
    return std::format("${}\r\n{}\r\n", str.size(), str);
}

inline auto simple_string(std::string_view str) -> std::string
{
    return std::format("+{}\r\n", str);
}

inline auto integer(std::int64_t n) -> std::string
{
    return std::format(":{}\r\n", n);
}

inline auto array_count(std::int64_t n) -> std::string
{
    return std::format("*{}\r\n", n);
}

} // namespace resp

} // namespace crispy::redis

#endif // CRISPY_REDIS_MESSAGE_H