module;

#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

export module cas.storage.fs:concepts;

import cas.result;

namespace cas::storage::fs {

// Writes the content of an object to the disk, possibly transforming it on the way.
export template<typename T>
concept writer = requires(T w, std::span<std::byte const> data) {
    { w.write(data) } -> std::same_as<result::result<void>>;

    // makes sure that everything is written to the disk;
    // writer cannot be used after this function is invoked
    { std::move(w).finalize() } -> std::same_as<result::result<void>>;
};

} // namespace cas::storage::fs
