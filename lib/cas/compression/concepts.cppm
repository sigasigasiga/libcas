module;

#include <concepts>
#include <cstddef>
#include <span>
#include <vector>

export module cas.compression:concepts;

namespace cas::compression {

export template<typename T>
concept compressor =
    std::default_initializable<T> &&
    requires(
        T c,
        std::span<std::byte const> in,
        std::vector<std::byte> &out
    ) {
        // appends the compressed data produced so far to `out`
        c.feed(in, out);

        // appends the rest of the compressed data to `out`;
        // compressor cannot be used after this function is invoked
        std::move(c).finalize(out);
    };

} // namespace cas::compression
