module;

#include <span>

export module cas.hash:feed;

import :concepts;

namespace cas::hash {

export void feed(hash auto &h, std::span<std::byte const> in) {
    h.feed(in);
}

export void feed(hash auto &h, std::span<char const> in) {
    h.feed(std::as_bytes(in));
}

export void feed(hash auto &h, std::span<unsigned char const> in) {
    h.feed(std::as_bytes(in));
}

} // namespace cas::hash
