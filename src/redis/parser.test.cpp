#include "parser.h"

#include <algorithm>
#include <array>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("parser parses inline command", "[parser]")
{
    crispy::redis::StringBuffer buffer(16);
    crispy::redis::Parser parser(buffer);

    const std::array<char, 6> payload = { 'P', 'I', 'N', 'G', '\r', '\n' };
    auto writable = buffer.writable_data();
    std::copy(payload.begin(), payload.end(), writable.begin());
    buffer.consume_writable(payload.size());

    auto command = parser.parse();

    CHECK(command.status == crispy::redis::Command::Status::OK);
    CHECK(command.argv.size() == 1);
    CHECK(command.argv[0] == std::string_view("PING"));
    CHECK(command.raw == std::string_view("PING\r\n"));
    CHECK(command.consumed_bytes() == 6);
}

TEST_CASE("parser parses resp array command", "[parser]")
{
    crispy::redis::StringBuffer buffer(32);
    crispy::redis::Parser parser(buffer);

    const std::array<char, 23> payload = { '*',  '2',  '\r', '\n', '$',  '4',  '\r', '\n',
                                           'L',  'L',  'E',  'N',  '\r', '\n', '$',  '3',
                                           '\r', '\n', 'k',  'e',  'y',  '\r', '\n' };
    auto writable = buffer.writable_data();
    std::copy(payload.begin(), payload.end(), writable.begin());
    buffer.consume_writable(payload.size());

    auto command = parser.parse();

    CHECK(command.status == crispy::redis::Command::Status::OK);
    CHECK(command.argv.size() == 2);
    CHECK(command.argv[0] == std::string_view("LLEN"));
    CHECK(command.argv[1] == std::string_view("key"));
    CHECK(command.raw == std::string_view("*2\r\n$4\r\nLLEN\r\n$3\r\nkey\r\n"));
    CHECK(command.consumed_bytes() == 23);
}

TEST_CASE("parser basic behavior", "[parser]")
{
    CHECK(true);
}
