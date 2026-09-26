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
        std::span<std::byte, T::output_size> out
    ) {
        // either the size of the digest or `std::dynamic_extent`
        { T::output_size } -> std::same_as<std::size_t>;

        // if `T::output_size == std::dynamic_extent`, then return the size of the digest
        // else return `T::output_size`
        { h.digest_size() } -> std::same_as<std::size_t>;

        h.feed(in);

        h.finalize(out);
    };

} // namespace cas::hash
