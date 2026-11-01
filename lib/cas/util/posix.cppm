module;

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif

export module cas.util:posix;

namespace cas::util {

// Whether the code is compiled for a POSIX system.
export constexpr bool is_posix =
#ifdef _POSIX_VERSION
    true;
#else
    false;
#endif

} // namespace cas::util
