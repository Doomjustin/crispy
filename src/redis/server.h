#ifndef CRISPY_REDIS_SERVER_H
#define CRISPY_REDIS_SERVER_H

#include <vector>

#include <boost/asio.hpp>

#include "database.h"

namespace crispy::redis {

class Server {
private:
    boost::asio::io_context context_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::vector<Database> databases_;

public:
    explicit Server(std::uint16_t port);

    void run();

private:
    auto accept() -> boost::asio::awaitable<void>;

    auto session(boost::asio::ip::tcp::socket socket) -> boost::asio::awaitable<void>;

    auto shutdown_monitor() -> boost::asio::awaitable<void>;
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_SERVER_H
