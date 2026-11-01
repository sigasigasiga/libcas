module;

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <span>
#include <utility>

#include <unistd.h>

export module cas.storage.fs:file_writer;

import cas.result;
import cas.util;

namespace cas::storage::fs {

// Writes the data to a file as is.
export class file_writer
{
public:
    explicit file_writer(util::unique_file file)
        : m_file(std::move(file))
    {
        assert(m_file);
    }

public: // writer
    result::result<void> write(std::span<std::byte const> data) {
        if (data.empty()) {
            return {};
        }

        // ISO C does not require the stdio functions to set `errno` on failure, but POSIX does
        static_assert(util::is_posix, "fs::file_writer relies on stdio functions setting `errno`");

        if (std::fwrite(data.data(), 1, data.size(), m_file.get()) != data.size()) {
            return std::unexpected(util::errno_error("fs::file_writer: fwrite failed"));
        }

        return {};
    }

    result::result<void> finalize() && {
        if (std::fflush(m_file.get()) != 0) {
            return std::unexpected(util::errno_error("fs::file_writer: fflush failed"));
        }

        // `fflush` only hands the data over to the OS,
        // so without `fsync` an object may be lost or truncated on a power loss
        if (::fsync(::fileno(m_file.get())) != 0) {
            return std::unexpected(util::errno_error("fs::file_writer: fsync failed"));
        }

        // the file is released even if `fclose` fails
        if (std::fclose(m_file.release()) != 0) {
            return std::unexpected(util::errno_error("fs::file_writer: fclose failed"));
        }

        return {};
    }

private:
    util::unique_file m_file;
};

} // namespace cas::storage::fs
