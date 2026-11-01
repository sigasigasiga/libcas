module;

#include <array>
#include <cstddef>
#include <expected>
#include <span>
#include <utility>

export module cas.storage:put;

import :concepts;
import :object_handle;

import cas.reader;
import cas.result;

namespace cas::storage {

inline constexpr std::size_t put_chunk_size = 64 * 1024;

export template<storage_writer Writer, reader::reader Reader>
auto put(Writer &writer, Reader &reader)
    -> result::result<object_handle>
{
    auto tx = writer.begin_transaction();
    if (!tx) {
        return std::unexpected(tx.error());
    }

    std::array<std::byte, put_chunk_size> chunk;

    while (true) {
        auto n = reader.read(chunk);
        if (!n) {
            return std::unexpected(n.error());
        }

        if (*n == 0) {
            break;
        }

        auto written = tx->write(std::span(chunk).first(*n));
        if (!written) {
            return std::unexpected(written.error());
        }
    }

    return std::move(*tx).commit();
}

} // namespace cas::storage
