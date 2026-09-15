#include "buffer.h"

#include <cassert>

namespace crispy::redis {

StringBuffer::StringBuffer(std::size_t initial_size)
  : data_(initial_size, '\0')
{}

void StringBuffer::consume(std::size_t len) noexcept
{
    assert(len <= readable_size());

    if (len < readable_size())
        read_index_ += len;
    else
        consume_all();
}

void StringBuffer::consume_all() noexcept
{
    read_index_ = 0;
    write_index_ = 0;
}

auto StringBuffer::find(std::string_view delimeter) const noexcept
    -> std::optional<std::string_view>
{
    auto pos = peek().find(delimeter);

    if (pos == std::string_view::npos)
        return {};

    return peek().substr(0, pos);
}

auto StringBuffer::find(std::size_t from, std::string_view delimeter) const noexcept
    -> std::optional<std::string_view>
{
    assert(from <= readable_size());

    auto n = peek().substr(from).find(delimeter);

    if (n == std::string_view::npos)
        return {};

    return peek().substr(from, n);
}

} // namespace crispy::redis