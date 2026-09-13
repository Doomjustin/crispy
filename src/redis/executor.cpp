#include "executor.h"

#include <functional>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

#include "cast.h"
#include "client.h"
#include "database.h"
#include "message.h"

using namespace std::chrono;

namespace crispy::redis {

std::unordered_map<std::string, Executor::Handler> Executor::commands_{
    { "ping", &Executor::ping },         { "type", &Executor::type },
    { "dbsize", &Executor::dbsize },     { "flushdb", &Executor::flushdb },
    { "flushall", &Executor::flushall }, { "exists", &Executor::exists },
    { "select", &Executor::select },     { "set", &Executor::set },
    { "get", &Executor::get },           { "del", &Executor::del },
    { "lpush", &Executor::lpush },       { "rpush", &Executor::rpush },
    { "expire", &Executor::expire },     { "pexpire", &Executor::pexpire },
    { "ttl", &Executor::ttl },           { "pttl", &Executor::pttl },
    { "persist", &Executor::persist },   { "lpop", &Executor::lpop },
    { "rpop", &Executor::rpop },         { "lrange", &Executor::lrange },
};

Executor::Executor(Client& client)
  : client_{ client }
{}

void Executor::execute(Arguments argv)
{
    if (!argv.empty()) {
        auto cmd = to_lowercase(argv[0]);

        auto it = commands_.find(cmd);
        if (it == commands_.end()) {
            append(client_, resp::unknown_command(cmd));
            return;
        }

        it->second(client_, argv);
    }
}

void Executor::append(Client& client, std::string_view message)
{
    std::ranges::move(message, std::back_inserter(client.response_));
}

void Executor::ping(Client& client, Arguments argv)
{
    if (argv.size() == 1)
        append(client, resp::simple_string("PONG"));
    else if (argv.size() == 2)
        append(client, resp::bulk_string(argv[1]));
    else
        append(client, resp::wrong_number_of_arguments("ping"));
}

void Executor::type(Client& client, Arguments argv)
{
    if (argv.size() != 2) {
        append(client, resp::wrong_number_of_arguments("type"));
        return;
    }

    auto& db = client.current_db();
    auto* value = db.get(argv[1]);
    if (!value) {
        append(client, resp::simple_string("none"));
        return;
    }

    auto& str = *value;
    if (std::holds_alternative<std::string>(str))
        append(client, resp::simple_string("string"));
    else if (std::holds_alternative<List>(str))
        append(client, resp::simple_string("list"));
    else if (std::holds_alternative<Set>(str))
        append(client, resp::simple_string("set"));
    else if (std::holds_alternative<Hash>(str))
        append(client, resp::simple_string("hash"));
    else
        append(client, resp::simple_string("unknown"));
}

void Executor::dbsize(Client& client, Arguments argv)
{
    auto& db = client.current_db();
    append(client, resp::integer(db.size()));
}

void Executor::flushdb(Client& client, Arguments argv)
{
    auto& db = client.current_db();
    db.clear();
    append(client, OK);
}

void Executor::flushall(Client& client, Arguments argv)
{
    for (auto& db : client.databases())
        db.clear();

    append(client, OK);
}

void Executor::exists(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("exists"));
        return;
    }

    auto& db = client.current_db();

    std::size_t count = 0;
    for (std::size_t i = 1; i < argv.size(); ++i)
        count += db.contains(argv[i]) ? 1 : 0;

    append(client, resp::integer(count));
}

void Executor::select(Client& client, Arguments argv)
{
    if (argv.size() != 2) {
        append(client, resp::wrong_number_of_arguments("select"));
        return;
    }

    auto res = numeric_cast<std::size_t>(argv[1]);
    if (!res || *res >= client.databases().size()) {
        append(client, INVALID_DB_INDEX);
        return;
    }

    client.select(*res);
    append(client, OK);
}

void Executor::set(Client& client, Arguments argv)
{
    if (argv.size() < 3) {
        append(client, resp::wrong_number_of_arguments("set"));
        return;
    }

    auto key = std::string{ argv[1] };
    auto value = std::string{ argv[2] };

    if (argv.size() == 3)
        set_generic(client, std::move(key), std::move(value), "NX");
    else if (argv.size() == 4)
        set_generic(client, std::move(key), std::move(value), argv[3]);
    else if (argv.size() == 5)
        set_expire(client, std::move(key), std::move(value), argv[3], argv[4]);
    else
        append(client, SYNTAX_ERROR);
}

void Executor::get(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("get"));
        return;
    }

    auto& db = client.current_db();

    auto* value = db.get(argv[1]);
    if (!value) {
        append(client, NIL);
        return;
    }

    auto& str = *value;
    if (!std::holds_alternative<std::string>(str)) {
        append(client, WRONG_TYPE_OPERATION);
        return;
    }

    auto result = std::get<std::string>(str);
    append(client, resp::bulk_string(result));
}

void Executor::del(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("del"));
        return;
    }

    auto& db = client.current_db();

    std::size_t deleted_count = 0;
    for (std::size_t i = 1; i < argv.size(); ++i)
        deleted_count += db.erase(std::string{ argv[i] });

    append(client, resp::integer(deleted_count));
}

void Executor::expire(Client& client, Arguments argv)
{
    if (argv.size() < 3) {
        append(client, resp::wrong_number_of_arguments("expire"));
        return;
    }

    auto& db = client.current_db();

    auto res = numeric_cast<std::size_t>(argv[2]);
    if (!res) {
        append(client, NOT_AN_INTEGER_OR_OUT_OF_RANGE);
        return;
    }

    auto expiry = Clock::now() + seconds{ *res };
    auto expired = db.expire(argv[1], expiry);
    append(client, resp::integer(expired ? 1 : 0));
}

void Executor::pexpire(Client& client, Arguments argv)
{
    if (argv.size() < 3) {
        append(client, resp::wrong_number_of_arguments("pexpire"));
        return;
    }

    auto& db = client.current_db();

    auto res = numeric_cast<std::size_t>(argv[2]);
    if (!res) {
        append(client, NOT_AN_INTEGER_OR_OUT_OF_RANGE);
        return;
    }

    auto expiry = Clock::now() + milliseconds{ *res };
    auto expired = db.expire(argv[1], expiry);
    append(client, resp::integer(expired ? 1 : 0));
}

void Executor::ttl(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("ttl"));
        return;
    }

    auto& db = client.current_db();
    auto [exists, ttl] = db.ttl(argv[1]);

    if (!exists) {
        append(client, resp::integer(-2));
        return;
    }

    if (!ttl) {
        append(client, resp::integer(-1));
        return;
    }

    auto ttl_seconds = duration_cast<seconds>(*ttl - Clock::now()).count();
    append(client, resp::integer(ttl_seconds));
}

void Executor::pttl(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("pttl"));
        return;
    }

    auto& db = client.current_db();
    auto [exists, ttl] = db.ttl(argv[1]);

    if (!exists) {
        append(client, resp::integer(-2));
        return;
    }

    if (!ttl) {
        append(client, resp::integer(-1));
        return;
    }

    auto ttl_milliseconds = duration_cast<milliseconds>(*ttl - Clock::now()).count();
    append(client, resp::integer(ttl_milliseconds));
}

void Executor::persist(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("persist"));
        return;
    }

    auto& db = client.current_db();
    auto persisted = db.expire(argv[1], std::nullopt);
    append(client, resp::integer(persisted ? 1 : 0));
}

void Executor::lpush(Client& client, Arguments argv)
{
    if (argv.size() < 3) {
        append(client, resp::wrong_number_of_arguments("lpush"));
        return;
    }

    auto& db = client.current_db();

    auto key = std::string{ argv[1] };
    if (!db.contains(key))
        db.set(std::string{ key }, List{});

    auto* value = db.get(key);
    if (!value || !std::holds_alternative<List>(*value)) {
        append(client, WRONG_TYPE_OPERATION);
        return;
    }

    auto& list = std::get<List>(*value);
    for (std::size_t i = 2; i < argv.size(); ++i)
        list.emplace_back(argv[i]);

    append(client, resp::integer(list.size()));
}

void Executor::rpush(Client& client, Arguments argv)
{
    if (argv.size() < 3) {
        append(client, resp::wrong_number_of_arguments("rpush"));
        return;
    }

    auto& db = client.current_db();

    auto key = std::string{ argv[1] };
    if (!db.contains(key))
        db.set(std::string{ key }, List{});

    auto* value = db.get(key);
    if (!value || !std::holds_alternative<List>(*value)) {
        append(client, WRONG_TYPE_OPERATION);
        return;
    }

    auto& list = std::get<List>(*value);
    for (std::size_t i = 2; i < argv.size(); ++i)
        list.emplace_back(argv[i]);

    append(client, resp::integer(list.size()));
}

void Executor::lpop(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("lpop"));
        return;
    }

    std::size_t count = 1;
    if (argv.size() >= 3) {
        auto res = numeric_cast<std::size_t>(argv[2]);
        if (!res) {
            append(client, NOT_AN_INTEGER_OR_OUT_OF_RANGE);
            return;
        }

        count = *res;
    }

    auto& db = client.current_db();

    auto* value = db.get(argv[1]);
    if (!value) {
        append(client, NIL);
        return;
    }

    if (!std::holds_alternative<List>(*value)) {
        append(client, WRONG_TYPE_OPERATION);
        return;
    }

    auto& list = std::get<List>(*value);
    if (list.empty()) {
        append(client, NIL);
        return;
    }

    auto result = resp::array_count(std::min(count, list.size()));
    while (count-- > 0 && !list.empty()) {
        result += resp::bulk_string(list.back());
        list.pop_back();
    }

    append(client, result);
}

void Executor::rpop(Client& client, Arguments argv)
{
    if (argv.size() < 2) {
        append(client, resp::wrong_number_of_arguments("rpop"));
        return;
    }

    std::size_t count = 1;
    if (argv.size() >= 3) {
        auto res = numeric_cast<std::size_t>(argv[2]);
        if (!res) {
            append(client, NOT_AN_INTEGER_OR_OUT_OF_RANGE);
            return;
        }

        count = *res;
    }

    auto& db = client.current_db();

    auto* value = db.get(argv[1]);
    if (!value) {
        append(client, NIL);
        return;
    }

    if (!std::holds_alternative<List>(*value)) {
        append(client, WRONG_TYPE_OPERATION);
        return;
    }

    auto& list = std::get<List>(*value);
    if (list.empty()) {
        append(client, NIL);
        return;
    }

    auto result = resp::array_count(std::min(count, list.size()));
    while (count-- > 0 && !list.empty()) {
        result += resp::bulk_string(list.back());
        list.pop_back();
    }

    append(client, result);
}

void Executor::lrange(Client& client, Arguments argv)
{
    if (argv.size() < 4) {
        append(client, resp::wrong_number_of_arguments("lrange"));
        return;
    }

    auto& db = client.current_db();

    auto* value = db.get(argv[1]);
    if (!value) {
        append(client, NIL);
        return;
    }

    if (!std::holds_alternative<List>(*value)) {
        append(client, WRONG_TYPE_OPERATION);
        return;
    }

    auto& list = std::get<List>(*value);
    if (list.empty()) {
        append(client, NIL);
        return;
    }

    auto start_res = numeric_cast<std::size_t>(argv[2]);
    auto stop_res = numeric_cast<std::size_t>(argv[3]);
    if (!start_res || !stop_res) {
        append(client, NOT_AN_INTEGER_OR_OUT_OF_RANGE);
        return;
    }

    std::size_t start = *start_res;
    std::size_t stop = *stop_res;
    if (start > stop || start >= list.size()) {
        append(client, resp::array_count(0));
        return;
    }

    stop = std::min(stop, list.size() - 1);
    auto result = resp::array_count(stop - start + 1);
    for (std::size_t i = start; i <= stop; ++i)
        result += resp::bulk_string(list[i]);

    append(client, result);
}

void Executor::set_generic(Client& client,
                           std::string key,
                           std::string value,
                           std::string_view option)
{
    auto& db = client.current_db();

    auto opt = to_uppercase(option);

    if (opt == "NX") {
        db.set(std::move(key), std::move(value));
    }
    else if (opt == "XX") {
        if (db.contains(key))
            db.set(std::move(key), std::move(value));
    }
    else {
        append(client, SYNTAX_ERROR);
        return;
    }

    append(client, OK);
}

void Executor::set_expire(Client& client,
                          std::string key,
                          std::string value,
                          std::string_view option,
                          std::string_view expiry)
{
    auto& db = client.current_db();

    auto res = numeric_cast<std::uint64_t>(expiry);
    if (!res) {
        append(client, NOT_AN_INTEGER_OR_OUT_OF_RANGE);
        return;
    }

    auto when = Clock::now();
    auto opt = to_uppercase(option);

    if (opt == "EX") {
        when += seconds{ *res };
    }
    else if (opt == "PX") {
        when += milliseconds{ *res };
    }
    else {
        append(client, SYNTAX_ERROR);
        return;
    }

    db.set(std::move(key), std::move(value), when);
    append(client, OK);
}

} // namespace crispy::redis