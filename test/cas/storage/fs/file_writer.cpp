#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

import cas.storage.fs;
import cas.test.support;
import cas.util;

namespace fs = std::filesystem;

using cas::storage::fs::file_writer;
using cas::test::support::make_bytes;
using cas::test::support::read_file;
using cas::test::support::temp_dir;

namespace {

file_writer open_writer(fs::path const &path, char const *mode = "wbx") {
    cas::util::unique_file file(std::fopen(path.c_str(), mode));
    REQUIRE(file);
    return file_writer(std::move(file));
}

} // namespace

TEST_CASE("file_writer satisfies writer", "[storage][fs][file_writer]") {
    STATIC_REQUIRE(cas::storage::fs::writer<file_writer>);
}

TEST_CASE("file_writer writes the data as is", "[storage][fs][file_writer]") {
    temp_dir dir;
    auto const path = dir.path() / "file";
    auto writer = open_writer(path);

    SECTION("no data") {
        REQUIRE(std::move(writer).finalize());
        CHECK(read_file(path).empty());
    }

    SECTION("several chunks") {
        auto const data = make_bytes(1024 * 1024 + 123);
        auto const half = std::span(data).first(data.size() / 2);
        auto const rest = std::span(data).subspan(half.size());

        REQUIRE(writer.write(half));
        REQUIRE(writer.write({}));
        REQUIRE(writer.write(rest));
        REQUIRE(std::move(writer).finalize());

        CHECK(read_file(path) == data);
    }
}

TEST_CASE("file_writer can be moved", "[storage][fs][file_writer]") {
    temp_dir dir;
    auto const path = dir.path() / "file";
    auto const data = make_bytes(10);

    auto writer = open_writer(path);
    REQUIRE(writer.write(data));

    auto moved = std::move(writer);
    REQUIRE(moved.write(data));
    REQUIRE(std::move(moved).finalize());

    auto expected = data;
    expected.insert(expected.end(), data.begin(), data.end());
    CHECK(read_file(path) == expected);
}

TEST_CASE("file_writer reports errors", "[storage][fs][file_writer]") {
    temp_dir dir;
    auto const path = dir.path() / "file";

    std::ofstream(path).put('x');
    auto writer = open_writer(path, "rb");

    auto const written = writer.write(make_bytes(10));
    REQUIRE(!written);
    CHECK_THROWS_AS(written.error().throw_exception(), std::system_error);
}
