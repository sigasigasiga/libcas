module;

#include <filesystem>
#include <system_error>
#include <utility>

export module cas.storage.fs:temp_path;

namespace cas::storage::fs {

// Removes the file on destruction unless it is `release`d.
class temp_path
{
public:
    temp_path() = default;

    explicit temp_path(std::filesystem::path path)
        : m_path(std::move(path))
    {
    }

    temp_path(temp_path &&rhs) noexcept
        : m_path(std::exchange(rhs.m_path, {}))
    {
    }

    temp_path &operator=(temp_path &&rhs) noexcept {
        if (this != &rhs) {
            reset();
            m_path = std::exchange(rhs.m_path, {});
        }

        return *this;
    }

    ~temp_path() { reset(); }

public:
    std::filesystem::path const &get() const { return m_path; }

    void release() { m_path.clear(); }

private:
    void reset() noexcept {
        if (!m_path.empty()) {
            std::error_code ec;
            std::filesystem::remove(m_path, ec);
            m_path.clear();
        }
    }

private:
    std::filesystem::path m_path;
};

} // namespace cas::storage::fs
