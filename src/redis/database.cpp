#include "database.h"

#include <spdlog/spdlog.h>

namespace crispy::redis {

Database::Database()
{
    keys_.reserve(4096);
    data_.reserve(4096);
    expiry_.reserve(4096);
}

auto Database::contains(std::string_view key) const noexcept -> bool
{
    if (is_expired(key))
        return false;

    return data_.contains(key);
}

auto Database::get(std::string_view key) -> Value*
{
    auto it = data_.find(key);
    if (it == data_.end())
        return nullptr;

    if (is_expired(key)) {
        SPDLOG_DEBUG("Key '{}' is expired and will be removed", key);
        erase(key);
        return nullptr;
    }

    return &it->second;
}

auto Database::erase(std::string_view key) noexcept -> std::size_t
{
    auto erased_expiry_count = expiry_.erase(key);
    auto erased_data_count = data_.erase(key);
    if (erased_data_count != erased_expiry_count)
        SPDLOG_ERROR("Mismatch in erased data and expiry counts for key '{}': data={}, expiry={}",
                     key,
                     erased_data_count,
                     erased_expiry_count);

    auto it = std::ranges::find(keys_, key);
    if (it != keys_.end()) {
        SPDLOG_DEBUG("Key '{}' has been erased from the database", key);
        keys_.erase(it);
    }

    return erased_data_count;
}

auto Database::expire(std::string_view key, std::optional<TimePoint> expiry) -> bool
{
    if (!contains(key))
        return false;

    expiry_.insert_or_assign(key, expiry);
    return true;
}

auto Database::ttl(std::string_view key) const noexcept -> std::pair<bool, std::optional<TimePoint>>
{
    auto it = expiry_.find(key);
    if (it == expiry_.end())
        return { false, std::nullopt };

    return { true, it->second };
}

auto Database::is_expired(std::string_view key) const noexcept -> bool
{
    auto it = expiry_.find(key);
    if (it == expiry_.end() || !it->second.has_value())
        return false;

    return std::chrono::system_clock::now() > it->second.value();
}

} // namespace crispy::redis