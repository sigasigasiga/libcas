module;

#include <concepts>
#include <span>

export module cas.hash:concepts;

namespace cas::hash {

export template<typename T>
concept hash =
    std::default_initializable<T> &&
    requires(
        T h,
        std::span<std::byte const> in,
        std::span<std::byte> out
    ) {
        { h.digest_size() } -> std::same_as<std::size_t>;

        h.feed(in);

        // hasher cannot be used after this function is invoked
        h.finalize(out);
    };

} // namespace cas::hash
