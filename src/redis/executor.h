#ifndef CRISPY_REDIS_EXECUTOR_H
#define CRISPY_REDIS_EXECUTOR_H

#include <chrono>
#include <functional>
#include <span>
#include <string_view>
#include <unordered_map>

#include "database.h"

namespace crispy::redis {

class Client;

class Executor {
private:
    using Arguments = std::span<std::string_view>;
    using Handler = std::function<void(Client&, Arguments)>;
    using Clock = Database::Clock;
    using TimePoint = Clock::time_point;
    using List = Database::List;
    using Set = Database::Set;
    using Hash = Database::Hash;

    Client& client_;

    static std::unordered_map<std::string, Handler> commands_;

public:
    explicit Executor(Client& client);

    void execute(Arguments argv);

private:
    static void append(Client& client, std::string_view message);

    static void ping(Client& client, Arguments argv);

    static void type(Client& client, Arguments argv);

    static void dbsize(Client& client, Arguments argv);

    static void flushdb(Client& client, Arguments argv);

    static void flushall(Client& client, Arguments argv);

    static void exists(Client& client, Arguments argv);

    static void select(Client& client, Arguments argv);

    static void set(Client& client, Arguments argv);

    static void get(Client& client, Arguments argv);

    static void del(Client& client, Arguments argv);

    static void expire(Client& client, Arguments argv);

    static void pexpire(Client& client, Arguments argv);

    static void ttl(Client& client, Arguments argv);

    static void pttl(Client& client, Arguments argv);

    static void persist(Client& client, Arguments argv);

    static void lpush(Client& client, Arguments argv);

    static void rpush(Client& client, Arguments argv);

    static void lpop(Client& client, Arguments argv);

    static void rpop(Client& client, Arguments argv);

    static void lrange(Client& client, Arguments argv);

    static void
    set_generic(Client& client, std::string key, std::string value, std::string_view option);

    static void set_expire(Client& client,
                           std::string key,
                           std::string value,
                           std::string_view option,
                           std::string_view expiry);
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_EXECUTOR_H