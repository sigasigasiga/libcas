module;

#include <cerrno>
#include <system_error>

export module cas.util:errno_error;

namespace cas::util {

// Makes an exception out of the error currently stored in `errno`.
export std::system_error errno_error(char const *what) {
    return std::system_error(errno, std::generic_category(), what);
}

} // namespace cas::util
