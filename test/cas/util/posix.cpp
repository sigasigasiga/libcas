#include <catch2/catch_test_macros.hpp>

import cas.util;

TEST_CASE("is_posix is set on POSIX systems", "[util][posix]") {
#if defined(__linux__) || defined(__APPLE__)
    STATIC_REQUIRE(cas::util::is_posix);
#elif defined(_WIN32)
    STATIC_REQUIRE(!cas::util::is_posix);
#endif
}
