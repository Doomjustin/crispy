#ifndef CRISPY_REDIS_PARSER_H
#define CRISPY_REDIS_PARSER_H

#include <cstdint>
#include <string_view>
#include <vector>

#include "buffer.h"

namespace crispy::redis {

struct Command {
    enum class Status : std::uint8_t {
        OK,           // Command parsed successfully
        NeedMoreData, // More data is needed to complete the command
        ProtocolError // The command has a protocol error
    };

    std::vector<std::string_view> argv;
    std::string_view raw; // 原始命令
    Status status{ Status::OK };

    constexpr auto consumed_bytes() const noexcept -> std::size_t
    {
        return raw.size();
    }
};

class Parser {
private:
    StringBuffer& buffer_;

public:
    explicit Parser(StringBuffer& buffer);

    auto parse() -> Command;

private:
    auto parse_inline(std::string_view line) -> Command;
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_PARSER_H
