#include "parser.h"

#include <ranges>

#include "cast.h"

namespace crispy::redis {

Parser::Parser(StringBuffer& buffer)
  : buffer_{ buffer }
{}

auto Parser::parse() -> Command
{
    Command command{};
    std::size_t raw_size = 0;

    auto line_opt = buffer_.find("\r\n");
    if (!line_opt) {
        command.status = Command::Status::NeedMoreData;
        return command;
    }

    auto line = *line_opt;
    if (line.front() != '*')
        return parse_inline(line);

    raw_size = line.size() + 2; // Include the "\r\n" in the raw size

    auto res = numeric_cast<std::size_t>(line.substr(1));
    if (!res) {
        command.status = Command::Status::ProtocolError;
        return command;
    }

    auto argc = *res;
    // 提前为参数列表分配空间，避免在循环中频繁分配内存。
    command.argv.reserve(argc);

    // 解析每个参数，直到所有参数都被读取。
    // 我们认为每个参数都以"$<length>\r\n<content>\r\n"的形式存在。
    while (argc-- > 0) {
        // 读入参数的长度信息
        line_opt = buffer_.find(raw_size, "\r\n");
        if (!line_opt) {
            command.status = Command::Status::NeedMoreData;
            return command;
        }

        if (line_opt->front() != '$') {
            command.status = Command::Status::ProtocolError;
            return command;
        }

        raw_size += line_opt->size() + 2; // Include the "\r\n" in the raw size

        res = numeric_cast<std::size_t>(line_opt->substr(1));
        if (!res) {
            command.status = Command::Status::ProtocolError;
            return command;
        }

        auto length = *res;

        // 读入参数的内容
        line_opt = buffer_.find(raw_size, "\r\n");
        if (!line_opt) {
            command.status = Command::Status::NeedMoreData;
            return command;
        }

        // 检查参数的长度是否与声明的长度一致
        if (line_opt->size() != length) {
            command.status = Command::Status::ProtocolError;
            return command;
        }

        raw_size += line_opt->size() + 2; // Include the "\r\n" in the raw size
        command.argv.push_back(*line_opt);
    }

    command.raw = buffer_.view(raw_size);
    return command;
}

auto Parser::parse_inline(std::string_view line) -> Command
{
    Command command{};
    using namespace std::string_view_literals;
    for (auto arg : std::ranges::views::split(line, " "sv))
        command.argv.emplace_back(arg);

    command.raw = buffer_.view(line.size() + 2);
    return command;
}

} // namespace crispy::redis
