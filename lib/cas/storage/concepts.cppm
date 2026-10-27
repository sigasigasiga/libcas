module;

#include <cassert>
#include <cstddef>
#include <span>
#include <utility>

export module cas.storage:concepts;

import :object_handle;

import cas.hash;
import cas.result;

namespace cas::storage {

export template<typename T>
concept write_transaction =
    hash::hash<typename T::hash_type> &&
    requires (T v, std::span<std::byte const> chunk) {
        { v.write(chunk) } -> std::same_as<result::result<void>>;
        { std::move(v).commit() } -> std::same_as<result::result<object_handle>>;
    };

export template<typename T>
concept storage_writer =
    hash::hash<typename T::hash_type> &&
    write_transaction<typename T::transaction_type> &&
    requires (T v) {
        { v.begin_transaction() } -> std::same_as<result::result<typename T::transaction_type>>;
    };

} // namespace cas::storage
