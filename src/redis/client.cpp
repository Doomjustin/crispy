#include "client.h"

#include <ranges>
#include <string_view>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/write.hpp>
#include <spdlog/spdlog.h>

#include "buffer.h"
#include "cast.h"
#include "executor.h"

using namespace boost;

namespace crispy::redis {

auto Client::execute() -> Awaitable<bool>
{
    auto [ec, n] = co_await socket_.async_read_some(request_.writable_data());
    if (ec) {
        if (ec == asio::error::operation_aborted)
            SPDLOG_DEBUG("read operation canceled.");
        else if (ec == asio::error::eof)
            SPDLOG_DEBUG("client closed the connection (EOF).");
        else
            SPDLOG_ERROR("read failed: {}.", ec.message());

        co_return false;
    }

    request_.consume_writable(n);
    SPDLOG_DEBUG("received: {}", escaped(request_.view(n)));

    response_.clear();
    while (true) {
        auto cmd = parser_.parse();
        // 此时可能还没有完整的命令，需要继续读取数据。
        if (cmd.status == Command::Status::NeedMoreData)
            break;

        if (cmd.status == Command::Status::ProtocolError) {
            SPDLOG_ERROR("protocol error in command: {}.", cmd.raw);
            co_return false;
        }

        SPDLOG_DEBUG("request: {}", escaped(cmd.raw));

        executor_.execute(cmd.argv);
        request_.consume(cmd.consumed_bytes());
    }

    co_return co_await write();
}

auto Client::write() -> Awaitable<bool>
{
    SPDLOG_DEBUG("response: {}", escaped(response_));

    auto [ec, _] = co_await asio::async_write(socket_, asio::buffer(response_));
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