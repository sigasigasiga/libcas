module;

#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>

export module cas.hash:digest;

namespace cas::hash {

namespace detail {

constexpr char hex_digits[] = "0123456789abcdef";

constexpr std::byte from_hex_char(char c) {
    char ret;

    if ('0' <= c && c <= '9') {
        ret = c - '0';
    } else if ('a' <= c && c <= 'f') {
        ret = c - 'a' + 10;
    } else if ('A' <= c && c <= 'F') {
        ret = c - 'A' + 10;
    } else {
        throw std::invalid_argument(std::format("to_hex: `{:?}` is not a hex character", c));
    }

    return static_cast<std::byte>(ret);
}

} // namespace detail

export template<std::output_iterator<char> O>
constexpr O to_hex(std::span<std::byte const> digest, O out) {
    for (std::byte b : digest) {
        auto const v = std::to_integer<unsigned char>(b);
        *out++ = detail::hex_digits[v >> 4];
        *out++ = detail::hex_digits[v & 0x0f];
    }

    return out;
}

export template<std::output_iterator<std::byte> O>
constexpr O from_hex(std::string_view hex, O out) {
    if (hex.size() % 2 != 0) {
        throw std::invalid_argument("from_hex: input length is odd");
    }

    for (std::size_t i = 0; i < hex.size(); i += 2) {
        auto const hi = detail::from_hex_char(hex[i]);
        auto const lo = detail::from_hex_char(hex[i + 1]);
        *out++ = (hi << 4) | lo;
    }

    return out;
}

} // namespace cas::hash
