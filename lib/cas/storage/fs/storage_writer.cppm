module;

#include <array>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

export module cas.storage.fs:storage_writer;

import :concepts;
import :make_object_path;
import :temp_path;

import cas.hash;
import cas.result;
import cas.storage;
import cas.util;

namespace cas::storage::fs {

// Objects are addressed by the digest of the data passed to `write`,
// no matter how `Writer` transforms it on the way to the disk.
export template<hash::hash Hash, writer Writer>
class transaction
{
public:
    using hash_type = Hash;

public:
    transaction(std::filesystem::path root, temp_path temp, Writer writer)
        : m_root(std::move(root))
        , m_temp_path(std::move(temp))
        , m_writer(std::move(writer))
    {
    }

public: // write_transaction
    result::result<void> write(std::span<std::byte const> chunk) {
        auto written = m_writer.write(chunk);
        if (written) {
            m_hash.feed(chunk);
        }

        return written;
    }

    result::result<object_handle> commit() && {
        if (auto finalized = std::move(m_writer).finalize(); !finalized) {
            return std::unexpected(finalized.error());
        }

        namespace stdfs = std::filesystem;
        std::error_code ec;

        stdfs::permissions(
            m_temp_path.get(),
            stdfs::perms::owner_read | stdfs::perms::group_read | stdfs::perms::others_read,
            ec
        );
        if (ec) {
            return std::unexpected(std::system_error(ec, "fs::transaction: cannot make the object read-only"));
        }

        std::array<std::byte, hash::max_raw_digest_size> digest;
        auto const digest_size = m_hash.digest_size();
        std::move(m_hash).finalize(digest);

        object_handle handle(std::span(digest).first(digest_size));
        auto const path = make_object_path(m_root, handle);

        stdfs::create_directory(path.parent_path(), ec);
        if (ec) {
            return std::unexpected(std::system_error(ec, "fs::transaction: cannot create the object directory"));
        }

        // an existing object has the same content, so it is fine to replace it
        stdfs::rename(m_temp_path.get(), path, ec);
        if (ec) {
            return std::unexpected(std::system_error(ec, "fs::transaction: cannot rename the object"));
        }

        m_temp_path.release();
        return handle;
    }

private:
    std::filesystem::path m_root;
    temp_path m_temp_path;
    Writer m_writer;
    Hash m_hash;
};

// Stores objects in `root`, which must be an existing directory.
// Each object is written to its own file by a new `Writer`.
export template<hash::hash Hash, writer Writer>
class storage_writer
{
public:
    using hash_type = Hash;
    using transaction_type = transaction<Hash, Writer>;

public:
    explicit storage_writer(std::filesystem::path root)
        : m_root(std::move(root))
    {
    }

public:
    std::filesystem::path const &root() const { return m_root; }

public: // storage_writer
    result::result<transaction_type> begin_transaction() const {
        auto name = (m_root / "tmp_obj_XXXXXX").string();

        int const fd = ::mkostemp(name.data(), O_CLOEXEC);
        if (fd < 0) {
            return std::unexpected(util::errno_error("fs::storage_writer: mkostemp failed"));
        }

        temp_path path(std::move(name));

        util::unique_file file(::fdopen(fd, "wb"));
        if (!file) {
            auto error = util::errno_error("fs::storage_writer: fdopen failed");
            ::close(fd);
            return std::unexpected(std::move(error));
        }

        return transaction_type(m_root, std::move(path), Writer(std::move(file)));
    }

private:
    std::filesystem::path m_root;
};

} // namespace cas::storage::fs
