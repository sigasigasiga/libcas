module;

#include <filesystem>
#include <system_error>
#include <utility>

#include <stdlib.h>

export module cas.test.support:temp_dir;

import cas.util;

namespace cas::test::support {

// A unique directory which is removed with everything inside it on destruction.
export class temp_dir
{
public:
    temp_dir() {
        auto name = (std::filesystem::temp_directory_path() / "cas_test_XXXXXX").native();
        if (!::mkdtemp(name.data())) {
            throw util::errno_error("temp_dir: mkdtemp failed");
        }

        m_path = std::move(name);
    }

    temp_dir(temp_dir const &) = delete;
    temp_dir &operator=(temp_dir const &) = delete;

    ~temp_dir() {
        std::error_code ec;
        std::filesystem::remove_all(m_path, ec);
    }

public:
    std::filesystem::path const &path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

} // namespace cas::test::support
