#include <cerrno>
#include <string>
#include <system_error>

#include <catch2/catch_test_macros.hpp>

import cas.util;

TEST_CASE("errno_error captures the current errno", "[util][errno_error]") {
    errno = ENOENT;
    auto const error = cas::util::errno_error("something failed");

    CHECK(error.code() == std::errc::no_such_file_or_directory);
    CHECK(error.code().category() == std::generic_category());
    CHECK(std::string(error.what()).starts_with("something failed"));
}

TEST_CASE("errno_error accepts an explicit errno value", "[util][errno_error]") {
    errno = 0;
    auto const error = cas::util::errno_error(EACCES, "something failed");

    CHECK(error.code() == std::errc::permission_denied);
    CHECK(error.code().category() == std::generic_category());
}
