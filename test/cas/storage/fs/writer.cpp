#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <stdlib.h>

#include <zlib.h>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

import cas.compression;
import cas.hash;
import cas.reader;
import cas.storage;
import cas.storage.fs;

namespace fs = std::filesystem;

using cas::hash::to_hex;
using cas::storage::object_handle;
using cas::storage::put;
using cas::storage::fs::object_path;

namespace {

using plain_writer = cas::storage::fs::writer<cas::hash::sha256>;
using zlib_writer = cas::storage::fs::writer<cas::hash::sha256, cas::compression::zlib<>>;

constexpr std::string_view abc_hex = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

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

std::vector<std::byte> make_bytes(std::string_view s) {
    return std::vector(
        reinterpret_cast<std::byte const *>(s.data()),
        reinterpret_cast<std::byte const *>(s.data() + s.size())
    );
}

std::string to_hex_string(object_handle const &handle) {
    std::string ret;
    to_hex(handle.span(), std::back_inserter(ret));
    return ret;
}

std::vector<std::byte> read_file(fs::path const &path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in);

    std::vector<char> data{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    return make_bytes(std::string_view(data.data(), data.size()));
}

std::vector<std::byte> zlib_uncompress(std::span<std::byte const> data, std::size_t size) {
    std::vector<std::byte> ret(size);
    auto ret_size = static_cast<uLongf>(ret.size());

    int const res = ::uncompress(
        reinterpret_cast<Bytef *>(ret.data()),
        &ret_size,
        reinterpret_cast<Bytef const *>(data.data()),
        static_cast<uLong>(data.size())
    );

    REQUIRE(res == Z_OK);
    ret.resize(ret_size);
    return ret;
}

// Every entry of `root` relative to it, so that leftovers are easy to spot.
std::vector<std::string> list_entries(fs::path const &root) {
    std::vector<std::string> ret;
    for (auto const &entry : fs::recursive_directory_iterator(root)) {
        ret.push_back(entry.path().lexically_relative(root).string());
    }
    std::ranges::sort(ret);
    return ret;
}

template<typename Writer>
auto put_bytes(Writer &writer, std::vector<std::byte> const &data) {
    cas::reader::iterator_reader reader(data.begin(), data.end());
    return put(writer, reader);
}

} // namespace

TEMPLATE_TEST_CASE(
    "fs::writer satisfies storage_writer",
    "[storage][fs]",
    plain_writer,
    zlib_writer
) {
    STATIC_REQUIRE(cas::storage::storage_writer<TestType>);
}

TEST_CASE("object_path follows the git loose objects layout", "[storage][fs]") {
    std::vector<std::byte> digest;
    cas::hash::from_hex(abc_hex, std::back_inserter(digest));

    auto const path = object_path("root", object_handle(digest));

    CHECK(path == fs::path("root") / "ba" / "7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("fs::writer stores uncompressed objects", "[storage][fs]") {
    temp_dir dir;
    plain_writer writer(dir.path());

    SECTION("short input") {
        auto const handle = put_bytes(writer, make_bytes("abc"));

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == abc_hex);
        CHECK(read_file(object_path(dir.path(), *handle)) == make_bytes("abc"));
        CHECK(list_entries(dir.path()) == std::vector<std::string>{"ba", "ba/7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"});
    }

    SECTION("empty input") {
        auto const handle = put_bytes(writer, {});

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        CHECK(read_file(object_path(dir.path(), *handle)).empty());
    }

    SECTION("large input") {
        auto const data = make_bytes(1024 * 1024 + 123);
        auto const handle = put_bytes(writer, data);

        REQUIRE(handle);
        CHECK(read_file(object_path(dir.path(), *handle)) == data);
    }
}

TEST_CASE("fs::writer stores compressed objects addressed by their uncompressed content", "[storage][fs]") {
    temp_dir dir;
    zlib_writer writer(dir.path());

    SECTION("short input") {
        auto const handle = put_bytes(writer, make_bytes("abc"));

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == abc_hex);

        auto const stored = read_file(object_path(dir.path(), *handle));
        CHECK(stored != make_bytes("abc"));
        CHECK(zlib_uncompress(stored, 3) == make_bytes("abc"));
    }

    SECTION("compressible input") {
        auto const data = std::vector<std::byte>(1024 * 1024, std::byte{'a'});
        auto const handle = put_bytes(writer, data);

        REQUIRE(handle);

        auto const stored = read_file(object_path(dir.path(), *handle));
        CHECK(stored.size() < data.size() / 100);
        CHECK(zlib_uncompress(stored, data.size()) == data);
    }

    SECTION("same address as the uncompressed object") {
        temp_dir plain_dir;
        plain_writer plain(plain_dir.path());

        auto const data = make_bytes(200 * 1024);
        auto const compressed = put_bytes(writer, data);
        auto const uncompressed = put_bytes(plain, data);

        REQUIRE(compressed);
        REQUIRE(uncompressed);
        CHECK(to_hex_string(*compressed) == to_hex_string(*uncompressed));
    }
}

TEST_CASE("fs::writer stores each object once", "[storage][fs]") {
    temp_dir dir;
    plain_writer writer(dir.path());

    auto const first = put_bytes(writer, make_bytes("abc"));
    auto const second = put_bytes(writer, make_bytes("abc"));

    REQUIRE(first);
    REQUIRE(second);
    CHECK(to_hex_string(*first) == to_hex_string(*second));
    CHECK(read_file(object_path(dir.path(), *first)) == make_bytes("abc"));
    CHECK(list_entries(dir.path()).size() == 2);
}

TEST_CASE("fs::writer stores distinct objects side by side", "[storage][fs]") {
    temp_dir dir;
    plain_writer writer(dir.path());

    auto const a = put_bytes(writer, make_bytes("a"));
    auto const b = put_bytes(writer, make_bytes("b"));

    REQUIRE(a);
    REQUIRE(b);
    CHECK(read_file(object_path(dir.path(), *a)) == make_bytes("a"));
    CHECK(read_file(object_path(dir.path(), *b)) == make_bytes("b"));
}

TEST_CASE("fs::writer makes objects read-only", "[storage][fs]") {
    temp_dir dir;
    plain_writer writer(dir.path());

    auto const handle = put_bytes(writer, make_bytes("abc"));

    REQUIRE(handle);

    auto const perms = fs::status(object_path(dir.path(), *handle)).permissions();
    CHECK((perms & fs::perms::all) == (fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read));
}

TEST_CASE("fs::writer leaves nothing behind if the transaction is not committed", "[storage][fs]") {
    temp_dir dir;
    zlib_writer writer(dir.path());

    {
        auto tx = writer.begin_transaction();
        auto const data = make_bytes("abc");
        REQUIRE(tx.write(data));
        CHECK(!list_entries(dir.path()).empty());
    }

    CHECK(list_entries(dir.path()).empty());
}

TEST_CASE("fs::writer reports errors", "[storage][fs]") {
    temp_dir dir;

    SECTION("missing root") {
        plain_writer writer(dir.path() / "missing");

        auto tx = writer.begin_transaction();
        auto const data = make_bytes("abc");

        auto const written = tx.write(data);
        REQUIRE(!written);
        CHECK_THROWS_AS(written.error().throw_exception(), std::system_error);

        auto const handle = std::move(tx).commit();
        REQUIRE(!handle);
        CHECK_THROWS_AS(handle.error().throw_exception(), std::system_error);
    }

    SECTION("missing root without writes") {
        plain_writer writer(dir.path() / "missing");

        auto const handle = put_bytes(writer, {});
        REQUIRE(!handle);
        CHECK_THROWS_AS(handle.error().throw_exception(), std::system_error);
    }

    SECTION("object directory cannot be created") {
        plain_writer writer(dir.path());

        // a file occupies the place of the object directory
        std::ofstream(dir.path() / "ba").put('x');

        auto const handle = put_bytes(writer, make_bytes("abc"));
        REQUIRE(!handle);
        CHECK_THROWS_AS(handle.error().throw_exception(), std::system_error);
        CHECK(list_entries(dir.path()) == std::vector<std::string>{"ba"});
    }
}
