#ifndef CRISPY_REDIS_CAST_H
#define CRISPY_REDIS_CAST_H

#include <charconv>
#include <expected>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace crispy::redis {

template<typename T>
struct is_byte_span : std::false_type {};

template<typename T, std::size_t Extent>
struct is_byte_span<std::span<T, Extent>>
  : std::bool_constant<std::same_as<std::remove_cv_t<T>, std::byte>> {};

template<typename T>
inline constexpr bool is_byte_span_v = is_byte_span<std::remove_cvref_t<T>>::value;

inline auto as_string(std::span<const std::byte> data) -> std::string_view
{
    return { reinterpret_cast<const char*>(data.data()), data.size() };
}

inline auto as_string(std::span<std::byte> data) -> std::string_view
{
    return { reinterpret_cast<const char*>(data.data()), data.size() };
}

template<std::ranges::contiguous_range T>
    requires(!is_byte_span_v<T>)
auto as_string(const T& range) -> std::string_view
{
    return as_string(std::as_bytes(std::span{ range }));
}

template<std::ranges::contiguous_range T>
    requires(!std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<T>>> &&
             !is_byte_span_v<T>)
auto as_string(T& range) -> std::string_view
{
    return as_string(std::as_writable_bytes(std::span{ range }));
}

template<typename T>
auto numeric_cast(std::string_view str) -> std::expected<T, std::error_code>
{
    T value{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc{})
        return std::unexpected(std::make_error_code(ec));

    if (ptr != str.data() + str.size())
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    return value;
}

template<std::floating_point T>
auto string_cast(T value) -> std::expected<std::string, std::error_code>
{
    // 符号/小数点/指数("e+NNN") 余量
    constexpr std::size_t buffer_size =
        static_cast<std::size_t>(std::numeric_limits<T>::max_digits10) + 8;

    std::array<char, buffer_size> buffer{};

    auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (ec != std::errc{})
        return std::unexpected(std::make_error_code(ec));

    if (ptr == buffer.data())
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    return std::string(buffer.data(), static_cast<std::size_t>(ptr - buffer.data()));
}

auto to_uppercase(std::string_view input) -> std::string;

auto to_lowercase(std::string_view input) -> std::string;

template<typename... T>
struct Overload : T... {
    using T::operator()...;
};

template<typename... T>
Overload(T...) -> Overload<T...>;

auto escaped(std::string_view input) -> std::string;

} // namespace crispy::redis

#endif // CRISPY_REDIS_CAST_H
