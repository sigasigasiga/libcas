module;

#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

export module cas.storage.fs:concepts;

import cas.result;
import cas.util;

namespace cas::storage::fs {

// Writes the content of an object to its file, possibly transforming it on the way.
export template<typename T>
concept writer =
    std::constructible_from<T, util::unique_file> &&
    requires(T w, std::span<std::byte const> data) {
        // if this function fails, the writer is left in a valid but unspecified state,
        // so it may only be destroyed
        { w.write(data) } -> std::same_as<result::result<void>>;

        // makes sure that everything is written to the file;
        // writer cannot be used after this function is invoked
        { std::move(w).finalize() } -> std::same_as<result::result<void>>;
    };

} // namespace cas::storage::fs
