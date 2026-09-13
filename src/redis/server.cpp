#include "server.h"

#include <boost/asio.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system.hpp>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "client.h"

using namespace boost;

namespace crispy::redis {

Server::Server(std::uint16_t port)
  : acceptor_{ context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port) }
  , databases_(16)
{
#ifdef NDEBUG
    spdlog::set_level(spdlog::level::warn);
#else
    spdlog::set_level(spdlog::level::debug);
#endif

    int opt = 1;
    if (::setsockopt(acceptor_.native_handle(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        SPDLOG_ERROR("Failed to set SO_REUSEADDR");
        throw std::system_error(errno, std::generic_category(), "Failed to set SO_REUSEADDR");
    }

    acceptor_.listen();
    SPDLOG_INFO("Server is listening on {}: {}",
                acceptor_.local_endpoint().address().to_string(),
                acceptor_.local_endpoint().port());
}

void Server::run()
{
    asio::co_spawn(
        context_,
        [this]() -> asio::awaitable<void> { co_await shutdown_monitor(); },
        asio::detached);

    asio::co_spawn(
        context_, [this]() -> asio::awaitable<void> { co_await accept(); }, asio::detached);

    context_.run();
}

auto Server::accept() -> boost::asio::awaitable<void>
{
    boost::system::error_code ec;

    while (true) {
        auto socket =
            co_await acceptor_.async_accept(asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            if (ec == asio::error::operation_aborted) {
                SPDLOG_INFO("Accept operation aborted");
                co_return;
            }

            SPDLOG_ERROR("Failed to accept connection: {}", ec.message());
            continue;
        }

        socket.non_blocking(true);
        socket.set_option(asio::ip::tcp::no_delay(true));
        asio::co_spawn(
            context_,
            [this, socket = std::move(socket)]() mutable -> asio::awaitable<void> {
                co_await session(std::move(socket));
            },
            asio::detached);
    }
}

auto Server::session(boost::asio::ip::tcp::socket socket) -> boost::asio::awaitable<void>
{
    SPDLOG_INFO("New session started");
    Client client{ std::move(socket), databases_ };

    while (co_await client.execute())
        ;

    SPDLOG_INFO("Session with socket {} closed.", client.native_handle());
}

auto Server::shutdown_monitor() -> boost::asio::awaitable<void>
{
    asio::signal_set signals{ context_, SIGINT, SIGTERM };

    co_await signals.async_wait(asio::use_awaitable);
    SPDLOG_INFO("Shutdown signal received");
    context_.stop();
}

} // namespace crispy::redis
