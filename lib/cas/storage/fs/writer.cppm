module;

#include <array>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

export module cas.storage.fs:writer;

import :object_path;
import :temp_file;

import cas.compression;
import cas.hash;
import cas.result;
import cas.storage;

namespace cas::storage::fs {

// Objects are addressed by the digest of their uncompressed content,
// no matter which compressor was used to store them.
export template<hash::hash Hash, compression::compressor Compressor>
class transaction
{
public:
    using hash_type = Hash;

public:
    transaction(std::filesystem::path root, result::result<temp_file> file)
        : m_root(std::move(root))
        , m_file(std::move(file))
    {
    }

public: // write_transaction
    result::result<void> write(std::span<std::byte const> chunk) {
        if (!m_file) {
            return std::unexpected(m_file.error());
        }

        m_hash.feed(chunk);
        m_compressor.feed(chunk, m_buffer);
        return flush();
    }

    result::result<object_handle> commit() && {
        if (!m_file) {
            return std::unexpected(m_file.error());
        }

        std::move(m_compressor).finalize(m_buffer);
        if (auto flushed = flush(); !flushed) {
            return std::unexpected(flushed.error());
        }

        if (auto sealed = m_file->seal(); !sealed) {
            return std::unexpected(sealed.error());
        }

        std::array<std::byte, hash::max_raw_digest_size> digest;
        auto const digest_size = m_hash.digest_size();
        std::move(m_hash).finalize(digest);

        object_handle handle(std::span(digest).first(digest_size));
        auto const path = object_path(m_root, handle);

        std::error_code ec;
        std::filesystem::create_directory(path.parent_path(), ec);
        if (ec) {
            return std::unexpected(std::system_error(ec, "fs::transaction: cannot create the object directory"));
        }

        // objects are immutable, so an existing one is kept and the temporary file is dropped
        if (std::filesystem::exists(path, ec)) {
            return handle;
        }

        std::filesystem::rename(m_file->path(), path, ec);
        if (ec) {
            return std::unexpected(std::system_error(ec, "fs::transaction: cannot rename the object"));
        }

        m_file->release();
        return handle;
    }

private:
    result::result<void> flush() {
        auto written = m_file->write(m_buffer);
        m_buffer.clear();

        if (!written) {
            // the file no longer matches the hash, so the transaction cannot be committed
            m_file = std::unexpected(written.error());
        }

        return written;
    }

private:
    std::filesystem::path m_root;
    result::result<temp_file> m_file;
    Hash m_hash;
    Compressor m_compressor;
    std::vector<std::byte> m_buffer;
};

// Stores objects in `root`, which must be an existing directory.
export template<hash::hash Hash, compression::compressor Compressor = compression::none>
class writer
{
public:
    using hash_type = Hash;

public:
    explicit writer(std::filesystem::path root)
        : m_root(std::move(root))
    {
    }

public:
    std::filesystem::path const &root() const { return m_root; }

public: // storage_writer
    transaction<Hash, Compressor> begin_transaction() const {
        return transaction<Hash, Compressor>(m_root, make_temp_file(m_root));
    }

private:
    std::filesystem::path m_root;
};

} // namespace cas::storage::fs
