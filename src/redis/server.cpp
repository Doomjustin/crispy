#include "server.h"

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system.hpp>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include "client.h"
#include "random.h"

using namespace boost;

namespace crispy::redis {

Server::Server(std::uint16_t port)
  : port_{ port }
  , acceptor_{ context_ }
  , databases_(16)
{
    spdlog::cfg::load_env_levels();
}

void Server::run()
{
    listen();

    auto shutdown_task = [this]() -> asio::awaitable<void> { co_await shutdown_monitor(); };
    auto check_ttl_task = [this]() -> asio::awaitable<void> { co_await check_ttl(); };
    auto accept_task = [this]() -> asio::awaitable<void> { co_await accept(); };

    asio::co_spawn(context_, shutdown_task, asio::detached);
    asio::co_spawn(context_, check_ttl_task, asio::detached);
    asio::co_spawn(context_, accept_task, asio::detached);

    context_.run();
}

void Server::listen()
{
    asio::ip::tcp::endpoint endpoint{ asio::ip::tcp::v4(), port_ };
    acceptor_.open(endpoint.protocol());

    int opt = 1;
    if (::setsockopt(acceptor_.native_handle(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        SPDLOG_ERROR("Failed to set SO_REUSEPORT");
        throw std::system_error(errno, std::generic_category(), "Failed to set SO_REUSEPORT");
    }

    acceptor_.set_option(asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    SPDLOG_INFO("Server is listening on {}: {}",
                acceptor_.local_endpoint().address().to_string(),
                acceptor_.local_endpoint().port());
}

auto Server::accept() -> Awaitable<void>
{
    while (true) {
        auto [ec, socket] = co_await acceptor_.async_accept();

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

        auto session_task = [this, socket = std::move(socket)]() mutable -> asio::awaitable<void> {
            co_await session(std::move(socket));
        };

        asio::co_spawn(context_, std::move(session_task), asio::detached);
    }
}

auto Server::session(Socket socket) -> Awaitable<void>
{
    SPDLOG_INFO("New session started");
    Client client{ std::move(socket), databases_ };

    while (co_await client.execute())
        ;

    SPDLOG_INFO("Session with socket {} closed.", client.native_handle());
}

auto format_signal(int signal) -> std::string_view
{
    switch (signal) {
    case SIGINT:
        return "SIGINT";
    case SIGTERM:
        return "SIGTERM";
    default:
        return "UNKNOWN";
    }
}

auto Server::shutdown_monitor() -> Awaitable<void>
{
    SignalSet signals{ context_, SIGINT, SIGTERM };

    auto [ec, signal] = co_await signals.async_wait();
    if (ec)
        SPDLOG_ERROR("Error while waiting for shutdown signal: {}", ec.message());

    SPDLOG_INFO("{} received, shutting down.", format_signal(signal));
    context_.stop();
}

auto Server::check_ttl() -> Awaitable<void>
{
    using namespace std::chrono_literals;
    auto timer = SteadyTimer{ context_ };
    while (true) {
        timer.expires_after(100ms);
        auto [ec] = co_await timer.async_wait();
        if (ec) {
            if (ec == asio::error::operation_aborted) {
                SPDLOG_DEBUG("TTL check operation aborted");
                co_return;
            }

            // 虽然发生了错误，但是由于是一个后台任务，可以让他继续
            SPDLOG_ERROR("TTL check failed: {}", ec.message());
        }

        for (auto& db : databases_) {
            auto expired_ratio = check_expired_ratio(db, 20);
            while (expired_ratio > 0.25F && expired_ratio < 1.0F) {
                SPDLOG_DEBUG(
                    "TTL check returned {:0.2f}, greater than or equal to 0.25, resampling.",
                    expired_ratio);

                expired_ratio = check_expired_ratio(db, 20);
            }
        }
    }
}

auto Server::check_expired_ratio(Database& db, std::size_t sample_size) -> float
{
    auto db_size = db.size();
    if (db_size == 0)
        return 0.0F;

    std::size_t expired_count = 0;
    for (std::size_t i = 0; i < sample_size; ++i) {
        auto keys = db.keys();
        if (keys.empty())
            break;

        std::string_view key = random::choice(keys);
        if (db.is_expired(key)) {
            db.erase(key);
            ++expired_count;
        }
    }

    return static_cast<float>(expired_count) / static_cast<float>(db_size);
}

} // namespace crispy::redis
