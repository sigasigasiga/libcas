module;

#include <concepts>
#include <span>
#include <utility>

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
        { std::as_const(h).digest_size() } -> std::same_as<std::size_t>;

        h.feed(in);

        // `feed` cannot be used after this function is invoked
        std::move(h).finalize(out);
    };

} // namespace cas::hash
