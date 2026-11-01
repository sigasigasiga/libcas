module;

#include <cerrno>
#include <system_error>

export module cas.util:errno_error;

namespace cas::util {

// Makes an exception out of an `errno` value.
export std::system_error errno_error(int code, char const *what) {
    return std::system_error(code, std::generic_category(), what);
}

// Makes an exception out of the error currently stored in `errno`.
export std::system_error errno_error(char const *what) {
    return errno_error(errno, what);
}

} // namespace cas::util
