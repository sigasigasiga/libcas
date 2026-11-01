module;

#include <cstddef>
#include <string_view>
#include <vector>

export module cas.test.support:bytes;

namespace cas::test::support {

// Makes `size` bytes of a non-repeating pattern.
export std::vector<std::byte> make_bytes(std::size_t size) {
    std::vector<std::byte> ret(size);
    for (std::size_t i = 0; i < size; ++i) {
        ret[i] = static_cast<std::byte>(i * 31 + 7);
    }
    return ret;
}

export std::vector<std::byte> make_bytes(std::string_view s) {
    return std::vector(
        reinterpret_cast<std::byte const *>(s.data()),
        reinterpret_cast<std::byte const *>(s.data() + s.size())
    );
}

} // namespace cas::test::support
