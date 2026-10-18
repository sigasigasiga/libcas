#include <algorithm>
#include <cstddef>
#include <expected>
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

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

import cas.hash;
import cas.reader;
import cas.result;
import cas.storage;
import cas.storage.fs;

namespace fs = std::filesystem;

using cas::hash::to_hex;
using cas::storage::object_handle;
using cas::storage::put;
using cas::storage::fs::file_writer;
using cas::storage::fs::object_path;

namespace {

constexpr std::string_view abc_hex = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

// Stores the bitwise complement of the data, so that the file differs from what is hashed.
class inverting_writer
{
public:
    explicit inverting_writer(file_writer file)
        : m_file(std::move(file))
    {
    }

public: // writer
    cas::result::result<void> write(std::span<std::byte const> data) {
        m_buffer.assign(data.begin(), data.end());
        for (auto &b : m_buffer) {
            b = ~b;
        }
        return m_file.write(m_buffer);
    }

    cas::result::result<void> finalize() && {
        return std::move(m_file).finalize();
    }

private:
    file_writer m_file;
    std::vector<std::byte> m_buffer;
};

struct failure_state
{
    std::size_t fail_write_at = static_cast<std::size_t>(-1);
    bool fail_finalize = false;
};

// Fails as configured by the global `failures`.
failure_state failures;

class failing_writer
{
public:
    explicit failing_writer(file_writer file)
        : m_file(std::move(file))
    {
    }

public: // writer
    cas::result::result<void> write(std::span<std::byte const> data) {
        if (m_writes++ == failures.fail_write_at) {
            return std::unexpected(std::runtime_error("write failed"));
        }
        return m_file.write(data);
    }

    cas::result::result<void> finalize() && {
        if (failures.fail_finalize) {
            return std::unexpected(std::runtime_error("finalize failed"));
        }
        return std::move(m_file).finalize();
    }

private:
    file_writer m_file;
    std::size_t m_writes = 0;
};

using plain_storage = cas::storage::fs::storage_writer<cas::hash::sha256>;
using inverting_storage = cas::storage::fs::storage_writer<cas::hash::sha256, inverting_writer>;
using failing_storage = cas::storage::fs::storage_writer<cas::hash::sha256, failing_writer>;

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

std::vector<std::byte> invert(std::vector<std::byte> data) {
    for (auto &b : data) {
        b = ~b;
    }
    return data;
}

std::string to_hex_string(object_handle const &handle) {
    std::string ret;
    to_hex(handle.span(), std::back_inserter(ret));
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

// Every entry of `root` relative to it, so that leftovers are easy to spot.
std::vector<std::string> list_entries(fs::path const &root) {
    std::vector<std::string> ret;
    for (auto const &entry : fs::recursive_directory_iterator(root)) {
        ret.push_back(entry.path().lexically_relative(root).string());
    }
    std::ranges::sort(ret);
    return ret;
}

template<typename Storage>
auto put_bytes(Storage &storage, std::vector<std::byte> const &data) {
    cas::reader::iterator_reader reader(data.begin(), data.end());
    return put(storage, reader);
}

} // namespace

TEMPLATE_TEST_CASE(
    "fs::storage_writer satisfies storage_writer",
    "[storage][fs]",
    plain_storage,
    inverting_storage
) {
    STATIC_REQUIRE(cas::storage::storage_writer<TestType>);
}

TEST_CASE("object_path follows the git loose objects layout", "[storage][fs]") {
    std::vector<std::byte> digest;
    cas::hash::from_hex(abc_hex, std::back_inserter(digest));

    auto const path = object_path("root", object_handle(digest));

    CHECK(path == fs::path("root") / "ba" / "7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("fs::storage_writer stores objects", "[storage][fs]") {
    temp_dir dir;
    plain_storage storage(dir.path());

    SECTION("short input") {
        auto const handle = put_bytes(storage, make_bytes("abc"));

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == abc_hex);
        CHECK(read_file(object_path(dir.path(), *handle)) == make_bytes("abc"));
        CHECK(list_entries(dir.path()) == std::vector<std::string>{"ba", "ba/7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"});
    }

    SECTION("empty input") {
        auto const handle = put_bytes(storage, {});

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        CHECK(read_file(object_path(dir.path(), *handle)).empty());
    }

    SECTION("large input") {
        auto const data = make_bytes(1024 * 1024 + 123);
        auto const handle = put_bytes(storage, data);

        REQUIRE(handle);
        CHECK(read_file(object_path(dir.path(), *handle)) == data);
    }
}

TEST_CASE("fs::storage_writer addresses objects by the data before the writer transforms it", "[storage][fs]") {
    temp_dir dir;
    inverting_storage storage(dir.path());

    auto const data = make_bytes(200 * 1024);
    auto const handle = put_bytes(storage, data);

    REQUIRE(handle);
    CHECK(read_file(object_path(dir.path(), *handle)) == invert(data));

    temp_dir plain_dir;
    plain_storage plain(plain_dir.path());
    auto const plain_handle = put_bytes(plain, data);

    REQUIRE(plain_handle);
    CHECK(to_hex_string(*handle) == to_hex_string(*plain_handle));
}

TEST_CASE("fs::storage_writer stores each object once", "[storage][fs]") {
    temp_dir dir;
    plain_storage storage(dir.path());

    auto const first = put_bytes(storage, make_bytes("abc"));
    auto const second = put_bytes(storage, make_bytes("abc"));

    REQUIRE(first);
    REQUIRE(second);
    CHECK(to_hex_string(*first) == to_hex_string(*second));
    CHECK(read_file(object_path(dir.path(), *first)) == make_bytes("abc"));
    CHECK(list_entries(dir.path()).size() == 2);
}

TEST_CASE("fs::storage_writer stores distinct objects side by side", "[storage][fs]") {
    temp_dir dir;
    plain_storage storage(dir.path());

    auto const a = put_bytes(storage, make_bytes("a"));
    auto const b = put_bytes(storage, make_bytes("b"));

    REQUIRE(a);
    REQUIRE(b);
    CHECK(read_file(object_path(dir.path(), *a)) == make_bytes("a"));
    CHECK(read_file(object_path(dir.path(), *b)) == make_bytes("b"));
}

TEST_CASE("fs::storage_writer makes objects read-only", "[storage][fs]") {
    temp_dir dir;
    plain_storage storage(dir.path());

    auto const handle = put_bytes(storage, make_bytes("abc"));

    REQUIRE(handle);

    auto const perms = fs::status(object_path(dir.path(), *handle)).permissions();
    CHECK((perms & fs::perms::all) == (fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read));
}

TEST_CASE("fs::storage_writer leaves nothing behind if the transaction is not committed", "[storage][fs]") {
    temp_dir dir;
    plain_storage storage(dir.path());

    {
        auto tx = storage.begin_transaction();
        auto const data = make_bytes("abc");
        REQUIRE(tx.write(data));
        CHECK(!list_entries(dir.path()).empty());
    }

    CHECK(list_entries(dir.path()).empty());
}

TEST_CASE("fs::storage_writer reports filesystem errors", "[storage][fs]") {
    temp_dir dir;

    SECTION("missing root") {
        plain_storage storage(dir.path() / "missing");

        auto tx = storage.begin_transaction();
        auto const data = make_bytes("abc");

        auto const written = tx.write(data);
        REQUIRE(!written);
        CHECK_THROWS_AS(written.error().throw_exception(), std::system_error);

        auto const handle = std::move(tx).commit();
        REQUIRE(!handle);
        CHECK_THROWS_AS(handle.error().throw_exception(), std::system_error);
    }

    SECTION("missing root without writes") {
        plain_storage storage(dir.path() / "missing");

        auto const handle = put_bytes(storage, {});
        REQUIRE(!handle);
        CHECK_THROWS_AS(handle.error().throw_exception(), std::system_error);
    }

    SECTION("object directory cannot be created") {
        plain_storage storage(dir.path());

        // a file occupies the place of the object directory
        std::ofstream(dir.path() / "ba").put('x');

        auto const handle = put_bytes(storage, make_bytes("abc"));
        REQUIRE(!handle);
        CHECK_THROWS_AS(handle.error().throw_exception(), std::system_error);
        CHECK(list_entries(dir.path()) == std::vector<std::string>{"ba"});
    }
}

TEST_CASE("fs::storage_writer reports writer errors", "[storage][fs]") {
    temp_dir dir;
    failing_storage storage(dir.path());
    failures = {};

    SECTION("a failed write prevents the commit") {
        failures.fail_write_at = 1;

        auto tx = storage.begin_transaction();
        auto const data = make_bytes("abc");

        REQUIRE(tx.write(data));

        auto const written = tx.write(data);
        REQUIRE(!written);
        CHECK_THROWS_WITH(written.error().throw_exception(), "write failed");

        // the writer is not asked again once it has failed
        failures = {};

        auto const again = tx.write(data);
        REQUIRE(!again);
        CHECK_THROWS_WITH(again.error().throw_exception(), "write failed");

        auto const handle = std::move(tx).commit();
        REQUIRE(!handle);
        CHECK_THROWS_WITH(handle.error().throw_exception(), "write failed");
    }

    SECTION("finalize error") {
        failures.fail_finalize = true;

        auto const handle = put_bytes(storage, make_bytes("abc"));
        REQUIRE(!handle);
        CHECK_THROWS_WITH(handle.error().throw_exception(), "finalize failed");
    }

    CHECK(list_entries(dir.path()).empty());
    failures = {};
}
