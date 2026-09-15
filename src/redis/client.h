#ifndef CRISPY_REDIS_CLIENT_H
#define CRISPY_REDIS_CLIENT_H

#include <span>

#include <boost/asio.hpp>

#include "alias.h"
#include "buffer.h"
#include "database.h"
#include "executor.h"
#include "parser.h"

namespace crispy::redis {

class Client {
    friend class Executor;

private:
    Socket socket_;
    std::size_t current_db_ = 0;
    std::span<Database> databases_;

    StringBuffer request_;
    std::string response_;
    Executor executor_{ *this };
    Parser parser_{ request_ };

public:
    Client(Socket socket, std::span<Database> databases)
      : socket_{ std::move(socket) }
      , databases_{ databases }
    {
        response_.reserve(4096);
    }

    auto socket() -> Socket&
    {
        return socket_;
    }

    auto native_handle()
    {
        return socket_.native_handle();
    }

    auto execute() -> Awaitable<bool>;

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
    auto write() -> Awaitable<bool>;
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_CLIENT_H