#ifndef CRISPY_REDIS_ALIAS_H
#define CRISPY_REDIS_ALIAS_H

#include <boost/asio.hpp>

using AwaitableToken = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;

using Socket = AwaitableToken::as_default_on_t<boost::asio::ip::tcp::socket>;

using Acceptor = AwaitableToken::as_default_on_t<boost::asio::ip::tcp::acceptor>;

using SteadyTimer = AwaitableToken::as_default_on_t<boost::asio::steady_timer>;

using SignalSet = AwaitableToken::as_default_on_t<boost::asio::signal_set>;

template<typename T>
using Awaitable = boost::asio::awaitable<T>;

#endif // CRISPY_REDIS_ALIAS_H