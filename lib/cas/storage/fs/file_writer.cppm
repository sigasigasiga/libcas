module;

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <memory>
#include <span>
#include <system_error>

#include <unistd.h>

export module cas.storage.fs:file_writer;

import grace.fn.bind;

import cas.result;

namespace cas::storage::fs {

namespace detail {

std::system_error last_error(char const *what) {
    return std::system_error(errno, std::system_category(), what);
}

} // namespace detail

// Writes the data to a file as is.
export class file_writer
{
public:
    // takes the ownership of `file`
    explicit file_writer(std::FILE &file)
        : m_file(&file)
    {
    }

public: // writer
    result::result<void> write(std::span<std::byte const> data) {
        if (data.empty()) {
            return {};
        }

        if (std::fwrite(data.data(), 1, data.size(), m_file.get()) != data.size()) {
            return std::unexpected(detail::last_error("fs::file_writer: fwrite failed"));
        }

        return {};
    }

    result::result<void> finalize() && {
        if (std::fflush(m_file.get()) != 0) {
            return std::unexpected(detail::last_error("fs::file_writer: fflush failed"));
        }

        if (::fsync(::fileno(m_file.get())) != 0) {
            return std::unexpected(detail::last_error("fs::file_writer: fsync failed"));
        }

        // the file is released even if `fclose` fails
        if (std::fclose(m_file.release()) != 0) {
            return std::unexpected(detail::last_error("fs::file_writer: fclose failed"));
        }

        return {};
    }

private:
    std::unique_ptr<std::FILE, grace::fn::bind::lift_invocable<::fclose>> m_file;
};

} // namespace cas::storage::fs
