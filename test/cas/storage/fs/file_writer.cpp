#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <stdlib.h>

#include <catch2/catch_test_macros.hpp>

import cas.storage.fs;

namespace fs = std::filesystem;

using cas::storage::fs::file_writer;

namespace {

class temp_dir
{
public:
    temp_dir() {
        auto name = (fs::temp_directory_path() / "cas_fs_test_XXXXXX").string();
        REQUIRE(::mkdtemp(name.data()) != nullptr);
        m_path = name;
    }

    temp_dir(temp_dir const &) = delete;
    temp_dir &operator=(temp_dir const &) = delete;

    ~temp_dir() {
        std::error_code ec;
        fs::remove_all(m_path, ec);
    }

    fs::path const &path() const { return m_path; }

private:
    fs::path m_path;
};

std::vector<std::byte> make_bytes(std::size_t size) {
    std::vector<std::byte> ret(size);
    for (std::size_t i = 0; i < size; ++i) {
        ret[i] = static_cast<std::byte>(i * 31 + 7);
    }
    return ret;
}

std::vector<std::byte> read_file(fs::path const &path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in);

    std::vector<std::byte> ret;
    for (auto it = std::istreambuf_iterator<char>(in); it != std::istreambuf_iterator<char>(); ++it) {
        ret.push_back(static_cast<std::byte>(*it));
    }
    return ret;
}

file_writer open_writer(fs::path const &path, char const *mode = "wbx") {
    std::FILE *file = std::fopen(path.c_str(), mode);
    REQUIRE(file != nullptr);
    return file_writer(*file);
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
