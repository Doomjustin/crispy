#include "buffer.h"

#include <array>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("buffer basic behavior", "[buffer]")
{
    crispy::redis::StringBuffer buffer(16);

    const std::array<char, 12> payload = { 'P', 'I', 'N', 'G', '\r', '\n',
                                           'P', 'O', 'N', 'G', '\r', '\n' };

    auto writable = buffer.writable_data();
    std::copy(payload.begin(), payload.end(), writable.begin());
    buffer.consume_writable(payload.size());

    CHECK(buffer.readable_size() == payload.size());
    CHECK(buffer.peek() == std::string_view("PING\r\nPONG\r\n"));
    CHECK(buffer.find("\r\n") == std::string_view("PING"));
    CHECK(buffer.find(6, "\r\n") == std::string_view("PONG"));

    buffer.consume(6);
    CHECK(buffer.peek() == std::string_view("PONG\r\n"));

    buffer.consume_all();
    CHECK(buffer.readable_size() == 0);
    CHECK(buffer.writable_size() == 16);
}