#include "client.h"

#include <ranges>
#include <string_view>
#include <vector>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/write.hpp>
#include <spdlog/spdlog.h>

#include "buffer.h"
#include "cast.h"
#include "executor.h"

using namespace boost;

namespace crispy::redis {

auto Client::execute() -> asio::awaitable<bool>
{
    boost::system::error_code ec;
    auto n = co_await socket_.async_read_some(request_.writable_data(),
                                              asio::redirect_error(asio::use_awaitable, ec));

    if (ec) {
        if (ec == asio::error::operation_aborted)
            SPDLOG_DEBUG("read operation canceled.");
        else if (ec == asio::error::eof)
            SPDLOG_DEBUG("peer closed the connection (EOF).");
        else
            SPDLOG_ERROR("read failed: {}.", ec.message());

        co_return false;
    }

    request_.consume_writable(n);
    SPDLOG_DEBUG("received: {}", escaped(request_.view(n)));

    Command cmd{};
    response_.clear();
    while (true) {
        cmd = parse_resp();
        // 此时可能还没有完整的命令，需要继续读取数据。
        if (cmd.status == Client::Command::Status::NeedMoreData)
            break;

        if (cmd.status == Client::Command::Status::ProtocolError) {
            SPDLOG_ERROR("protocol error in command: {}.", cmd.raw);
            co_return false;
        }

        SPDLOG_DEBUG("request: {}", escaped(cmd.raw));

        executor_.execute(cmd.argv);
        request_.consume(cmd.raw.size());
    }

    co_return co_await write();
}

auto Client::parse_inline(std::string_view line) -> Command
{
    Command command{};
    using namespace std::string_view_literals;
    for (auto arg : std::ranges::views::split(line, " "sv))
        command.argv.emplace_back(arg);

    command.raw = request_.view(line.size() + 2);
    return command;
}

auto Client::parse_resp() -> Client::Command
{
    Command command{};
    std::size_t raw_size = 0;

    auto line = request_.find("\r\n");
    if (line.empty()) {
        command.status = Client::Command::Status::NeedMoreData;
        return command;
    }

    if (line.front() != '*')
        return parse_inline(line);

    raw_size = line.size() + 2; // Include the "\r\n" in the raw size

    auto res = numeric_cast<std::size_t>(line.substr(1));
    if (!res) {
        command.status = Client::Command::Status::ProtocolError;
        return command;
    }

    auto argc = *res;
    // 提前为参数列表分配空间，避免在循环中频繁分配内存。
    command.argv.reserve(argc);

    // 解析每个参数，直到所有参数都被读取。
    // 我们认为每个参数都以"$<length>\r\n<content>\r\n"的形式存在。
    while (argc-- > 0) {
        // 读入参数的长度信息
        line = request_.find(raw_size, "\r\n");
        if (line.empty()) {
            command.status = Client::Command::Status::NeedMoreData;
            return command;
        }

        if (line.front() != '$') {
            command.status = Client::Command::Status::ProtocolError;
            return command;
        }

        raw_size += line.size() + 2; // Include the "\r\n" in the raw size

        res = numeric_cast<std::size_t>(line.substr(1));
        if (!res) {
            command.status = Client::Command::Status::ProtocolError;
            return command;
        }

        auto length = *res;

        // 读入参数的内容
        line = request_.find(raw_size, "\r\n");
        if (line.empty()) {
            command.status = Client::Command::Status::NeedMoreData;
            return command;
        }

        // 检查参数的长度是否与声明的长度一致
        if (line.size() != length) {
            command.status = Client::Command::Status::ProtocolError;
            return command;
        }

        raw_size += line.size() + 2; // Include the "\r\n" in the raw size
        command.argv.push_back(line);
    }

    command.raw = request_.view(raw_size);
    return command;
}

auto Client::write() -> asio::awaitable<bool>
{
    SPDLOG_DEBUG("response: {}", escaped(response_));

    boost::system::error_code ec;

    co_await asio::async_write(socket_, asio::buffer(response_), asio::redirect_error(ec));
    if (ec) {
        if (ec == asio::error::operation_aborted)
            SPDLOG_DEBUG("write operation canceled.");
        else if (ec == asio::error::broken_pipe || ec == asio::error::connection_reset)
            SPDLOG_DEBUG("peer closed during write: {}.", ec.message());
        else
            SPDLOG_ERROR("write failed: {}.", ec.message());

        co_return false;
    }

    co_return true;
}

} // namespace crispy::redis