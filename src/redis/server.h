#ifndef CRISPY_REDIS_SERVER_H
#define CRISPY_REDIS_SERVER_H

#include <cstdint>
#include <vector>

#include <boost/asio.hpp>

#include "alias.h"
#include "database.h"

namespace crispy::redis {

class Server {
private:
    boost::asio::io_context context_;
    std::uint16_t port_;
    Acceptor acceptor_;

    std::vector<Database> databases_;

public:
    explicit Server(std::uint16_t port);

    void run();

private:
    void listen();

    auto accept() -> Awaitable<void>;

    auto session(Socket socket) -> Awaitable<void>;

    auto shutdown_monitor() -> Awaitable<void>;

    auto check_ttl() -> Awaitable<void>;

    static auto check_expired_ratio(Database& db, std::size_t sample_size) -> float;
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_SERVER_H
