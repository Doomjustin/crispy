#ifndef CRISPY_REDIS_DATABASE_H
#define CRISPY_REDIS_DATABASE_H

#include <chrono>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace crispy::redis {

class Database {
public:
    using List = std::vector<std::string>;
    using Set = std::set<std::string>;
    using Hash = std::unordered_map<std::string, std::string>;
    using Value = std::variant<std::string, List, Set, Hash>;
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

private:
    // 存储所有的键
    std::vector<std::string> keys_;
    // data_ stores the actual key-value pairs in the database.
    std::unordered_map<std::string_view, Value> data_;
    // expiry_ stores the expiration time for each key.
    std::unordered_map<std::string_view, std::optional<TimePoint>> expiry_;

public:
    Database();

    auto contains(std::string_view key) const noexcept -> bool;

    auto get(std::string_view key) -> Value*;

    template<typename T>
    void set(std::string key, T value, std::optional<TimePoint> expiry = {})
    {
        keys_.push_back(std::move(key));
        expiry_.insert_or_assign(keys_.back(), expiry);
        data_.insert_or_assign(keys_.back(), std::move(value));
    }

    auto erase(std::string_view key) noexcept -> std::size_t;

    void clear() noexcept
    {
        expiry_.clear();
        data_.clear();
        keys_.clear();
    }

    auto expire(std::string_view key, std::optional<TimePoint> expiry) -> bool;

    // @return: exists, expiration time (if any)
    auto ttl(std::string_view key) const noexcept -> std::pair<bool, std::optional<TimePoint>>;

    auto keys() noexcept -> std::span<std::string>
    {
        return keys_;
    }

    auto keys() const noexcept -> std::span<const std::string>
    {
        return keys_;
    }

    auto is_expired(std::string_view key) const noexcept -> bool;

    constexpr auto size() const noexcept -> std::size_t
    {
        return keys_.size();
    }

    constexpr auto empty() const noexcept -> bool
    {
        return keys_.empty();
    }
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_DATABASE_H