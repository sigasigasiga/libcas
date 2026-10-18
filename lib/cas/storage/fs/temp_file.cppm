module;

#include <cerrno>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

export module cas.storage.fs:temp_file;

import cas.result;

namespace cas::storage::fs {

namespace detail {

std::system_error last_error(char const *what) {
    return std::system_error(errno, std::system_category(), what);
}

} // namespace detail

// A file which is removed on destruction unless it is `release`d.
class temp_file
{
public:
    temp_file(int fd, std::filesystem::path path)
        : m_fd(fd)
        , m_path(std::move(path))
    {
    }

    temp_file(temp_file &&rhs) noexcept
        : m_fd(std::exchange(rhs.m_fd, -1))
        , m_path(std::exchange(rhs.m_path, {}))
    {
    }

    temp_file &operator=(temp_file &&rhs) noexcept {
        if (this != &rhs) {
            reset();
            m_fd = std::exchange(rhs.m_fd, -1);
            m_path = std::exchange(rhs.m_path, {});
        }

        return *this;
    }

    ~temp_file() { reset(); }

public:
    std::filesystem::path const &path() const { return m_path; }

    result::result<void> write(std::span<std::byte const> data) {
        while (!data.empty()) {
            auto const n = ::write(m_fd, data.data(), data.size());
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }

                return std::unexpected(detail::last_error("fs::temp_file: write failed"));
            }

            data = data.subspan(static_cast<std::size_t>(n));
        }

        return {};
    }

    // Flushes the data to the disk, makes the file read-only and closes it.
    result::result<void> seal() {
        if (::fsync(m_fd) != 0) {
            return std::unexpected(detail::last_error("fs::temp_file: fsync failed"));
        }

        if (::fchmod(m_fd, 0444) != 0) {
            return std::unexpected(detail::last_error("fs::temp_file: fchmod failed"));
        }

        // the descriptor is released even if `close` fails
        if (::close(std::exchange(m_fd, -1)) != 0) {
            return std::unexpected(detail::last_error("fs::temp_file: close failed"));
        }

        return {};
    }

    // Keeps the file on the disk after destruction.
    void release() { m_path.clear(); }

private:
    void reset() noexcept {
        if (m_fd >= 0) {
            ::close(std::exchange(m_fd, -1));
        }

        if (!m_path.empty()) {
            std::error_code ec;
            std::filesystem::remove(m_path, ec);
            m_path.clear();
        }
    }

private:
    int m_fd;
    std::filesystem::path m_path;
};

result::result<temp_file> make_temp_file(std::filesystem::path const &dir) {
    auto name = (dir / "tmp_obj_XXXXXX").string();

    int const fd = ::mkostemp(name.data(), O_CLOEXEC);
    if (fd < 0) {
        return std::unexpected(detail::last_error("fs::make_temp_file: mkostemp failed"));
    }

    return temp_file(fd, std::move(name));
}

} // namespace cas::storage::fs
