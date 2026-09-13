#ifndef CRISPY_REDIS_BUFFER_H
#define CRISPY_REDIS_BUFFER_H

#include <cassert>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace crispy::redis {

class StringBuffer {
private:
    // +-------------------+------------------+------------------+
    // |   Prependable     |  Readable Bytes  |  Writable Bytes  |
    // |   (已读/预留头)    |    (等待被读取)   |   (还可以写入)   |
    // +-------------------+------------------+------------------+
    // 0 <= read_index_  <=  write_index_  <=  capacity

    std::string data_;
    std::size_t read_index_ = 0;
    std::size_t write_index_ = 0;

public:
    explicit StringBuffer(std::size_t initial_size = 4096);

    constexpr auto readable_size() const noexcept -> std::size_t
    {
        return write_index_ - read_index_;
    }

    constexpr auto writable_size() const noexcept -> std::size_t
    {
        return data_.size() - write_index_;
    }

    auto writable_data() noexcept -> std::span<char>
    {
        return { data_.data() + write_index_, writable_size() };
    }

    // 返回可读的数据区间
    auto peek() const noexcept -> std::string_view
    {
        return { data_.data() + read_index_, readable_size() };
    }

    auto view(std::size_t len) const noexcept -> std::string_view
    {
        assert(len <= readable_size());
        return { data_.data() + read_index_, len };
    }

    void consume_writable(std::size_t len) noexcept
    {
        write_index_ += len;
    }

    void consume(std::size_t len) noexcept;

    void consume_all() noexcept;

    auto find(std::string_view delimeter) const noexcept -> std::string_view;

    auto find(std::size_t from, std::string_view delimeter) const noexcept -> std::string_view;
};

} // namespace crispy::redis

#endif // CRISPY_REDIS_BUFFER_H