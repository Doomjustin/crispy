#ifndef CRISPY_REDIS_CLIENT_H
#define CRISPY_REDIS_CLIENT_H

#include <cstdint>
#include <span>

#include <boost/asio.hpp>

#include "buffer.h"
#include "database.h"
#include "executor.h"

namespace crispy::redis {

class Client {
    friend class Executor;

private:
    struct Command {
        enum class Status : std::uint8_t {
            OK,           // Command parsed successfully
            NeedMoreData, // More data is needed to complete the command
            ProtocolError // The command has a protocol error
        };

        std::vector<std::string_view> argv;
        std::string_view raw; // 原始命令
        Status status{ Status::OK };
    };

    boost::asio::ip::tcp::socket socket_;
    std::size_t current_db_ = 0;
    std::span<Database> databases_;

    StringBuffer request_;
    std::string response_;
    Executor executor_{ *this };

public:
    Client(boost::asio::ip::tcp::socket socket, std::span<Database> databases)
      : socket_{ std::move(socket) }
      , databases_{ databases }
    {
        response_.reserve(4096);
    }

    auto socket() -> boost::asio::ip::tcp::socket&
    {
        return socket_;
    }

    auto native_handle()
    {
        return socket_.native_handle();
    }

    auto execute() -> boost::asio::awaitable<bool>;

    auto current_db() noexcept -> Database&
    {
        return databases_[current_db_];
    }

    void select(std::size_t index)
    {
        current_db_ = index;
    }

    auto databases() noexcept -> std::span<Database>
    {
        return databases_;
    }

private:
    auto parse_inline(std::string_view line) -> Command;

    auto parse_resp() -> Command;

    auto write() -> boost::asio::awaitable<bool>;
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_CLIENT_H