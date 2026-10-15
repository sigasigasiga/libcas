module;

#include <concepts>
#include <cstddef>
#include <span>

export module cas.reader:concepts;

import cas.result;

namespace cas::reader {

export template<typename T>
concept reader = requires(T v, std::span<std::byte> out) {
    { v.read(out) } -> std::same_as<result::result<std::size_t>>;
};

} // namespace cas::reader
