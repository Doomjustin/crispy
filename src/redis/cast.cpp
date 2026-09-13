#include "cast.h"

namespace crispy::redis {

auto to_uppercase(std::string_view input) -> std::string
{
    std::string result{ input };
    for (char& c : result)
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - ('a' - 'A'));
    return result;
}

auto to_lowercase(std::string_view input) -> std::string
{
    std::string result{ input };
    for (char& c : result)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c + ('a' - 'A'));
    return result;
}

auto escaped(std::string_view input) -> std::string
{
    std::string result;

    for (char c : input) {
        switch (c) {
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\"':
            result += "\\\"";
            break;
        default:
            result += c;
            break;
        }
    }

    return result;
}

} // namespace crispy::redis