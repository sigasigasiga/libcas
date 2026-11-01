module;

#include <cstdio>
#include <memory>

export module cas.util:unique_file;

namespace cas::util {

export struct file_deleter
{
    static void operator()(std::FILE *file) noexcept {
        std::fclose(file);
    }
};

export using unique_file = std::unique_ptr<std::FILE, file_deleter>;

} // namespace cas::util
