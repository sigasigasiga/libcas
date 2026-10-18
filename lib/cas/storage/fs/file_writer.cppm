module;

#include <cerrno>
#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <utility>

#include <unistd.h>

export module cas.storage.fs:file_writer;

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
    // takes the ownership of `fd`
    explicit file_writer(int fd)
        : m_fd(fd)
    {
    }

    file_writer(file_writer &&rhs) noexcept
        : m_fd(std::exchange(rhs.m_fd, -1))
    {
    }

    file_writer &operator=(file_writer &&rhs) noexcept {
        if (this != &rhs) {
            reset();
            m_fd = std::exchange(rhs.m_fd, -1);
        }

        return *this;
    }

    ~file_writer() { reset(); }

public: // writer
    result::result<void> write(std::span<std::byte const> data) {
        while (!data.empty()) {
            auto const n = ::write(m_fd, data.data(), data.size());
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }

                return std::unexpected(detail::last_error("fs::file_writer: write failed"));
            }

            data = data.subspan(static_cast<std::size_t>(n));
        }

        return {};
    }

    result::result<void> finalize() && {
        if (::fsync(m_fd) != 0) {
            return std::unexpected(detail::last_error("fs::file_writer: fsync failed"));
        }

        // the descriptor is released even if `close` fails
        if (::close(std::exchange(m_fd, -1)) != 0) {
            return std::unexpected(detail::last_error("fs::file_writer: close failed"));
        }

        return {};
    }

private:
    void reset() noexcept {
        if (m_fd >= 0) {
            ::close(std::exchange(m_fd, -1));
        }
    }

private:
    int m_fd;
};

} // namespace cas::storage::fs
