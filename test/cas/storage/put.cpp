#include <algorithm>
#include <cstddef>
#include <expected>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

import cas.hash;
import cas.reader;
import cas.result;
import cas.storage;

using cas::hash::to_hex;
using cas::storage::object_handle;
using cas::storage::put;

namespace {

struct writer_state
{
    std::vector<std::vector<std::byte>> chunks;
    std::size_t transactions = 0;
    std::size_t commits = 0;
    std::size_t fail_write_at = static_cast<std::size_t>(-1);
};

class fake_transaction
{
public:
    using hash_type = cas::hash::sha256;

public:
    explicit fake_transaction(writer_state &state)
        : m_state(&state)
    {
    }

public: // write_transaction
    cas::result::result<void> write(std::span<std::byte const> chunk) {
        if (m_state->chunks.size() == m_state->fail_write_at) {
            return std::unexpected(std::runtime_error("write failed"));
        }

        m_state->chunks.emplace_back(chunk.begin(), chunk.end());
        m_hash.feed(chunk);
        return {};
    }

    cas::result::result<object_handle> commit() && {
        ++m_state->commits;

        std::byte digest[cas::hash::max_raw_digest_size];
        std::move(m_hash).finalize(digest);
        return object_handle(std::span(digest, m_hash.digest_size()));
    }

private:
    writer_state *m_state;
    hash_type m_hash;
};

class fake_writer
{
public:
    using hash_type = cas::hash::sha256;

    explicit fake_writer(writer_state &state)
        : m_state(&state)
    {
    }

public: // storage_writer
    fake_transaction begin_transaction() {
        ++m_state->transactions;
        return fake_transaction(*m_state);
    }

private:
    writer_state *m_state;
};

// Yields `data` and then fails instead of reporting the end of input.
class failing_reader
{
public:
    explicit failing_reader(std::vector<std::byte> data)
        : m_data(std::move(data))
    {
    }

public: // reader
    cas::result::result<std::size_t> read(std::span<std::byte> out) {
        if (m_done) {
            return std::unexpected(std::runtime_error("read failed"));
        }

        m_done = true;
        REQUIRE(m_data.size() < out.size());
        std::ranges::copy(m_data, out.begin());
        return m_data.size();
    }

private:
    std::vector<std::byte> m_data;
    bool m_done = false;
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

auto put_bytes(fake_writer &writer, std::vector<std::byte> const &data) {
    cas::reader::iterator_reader reader(data.begin(), data.end());
    return put(writer, reader);
}

} // namespace

TEST_CASE("put writes the whole input and returns the committed handle", "[storage][put]") {
    writer_state state;
    fake_writer writer(state);

    SECTION("empty input") {
        auto const handle = put_bytes(writer, {});

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        CHECK(state.chunks.empty());
    }

    SECTION("short input") {
        auto const handle = put_bytes(writer, make_bytes("abc"));

        REQUIRE(handle);
        CHECK(to_hex_string(*handle) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        CHECK(std::ranges::equal(state.chunks | std::views::join, make_bytes("abc")));
    }

    CHECK(state.transactions == 1);
    CHECK(state.commits == 1);
}

TEST_CASE("put splits large input into multiple non-empty writes", "[storage][put]") {
    writer_state state;
    fake_writer writer(state);

    auto const data = make_bytes(1024 * 1024 + 123);
    auto const handle = put_bytes(writer, data);

    REQUIRE(handle);
    CHECK(state.chunks.size() > 1);
    for (auto const &chunk : state.chunks) {
        CHECK(!chunk.empty());
    }
    CHECK(std::ranges::equal(state.chunks | std::views::join, data));
    CHECK(state.commits == 1);
}

TEST_CASE("put propagates errors without committing", "[storage][put]") {
    writer_state state;
    fake_writer writer(state);

    SECTION("read error") {
        failing_reader reader(make_bytes("abc"));

        auto const handle = put(writer, reader);

        REQUIRE(!handle);
        CHECK_THROWS_WITH(handle.error().throw_exception(), "read failed");
        CHECK(std::ranges::equal(state.chunks | std::views::join, make_bytes("abc")));
    }

    SECTION("write error") {
        state.fail_write_at = 1;

        auto const handle = put_bytes(writer, make_bytes(1024 * 1024));

        REQUIRE(!handle);
        CHECK_THROWS_WITH(handle.error().throw_exception(), "write failed");
        CHECK(state.chunks.size() == 1);
    }

    CHECK(state.transactions == 1);
    CHECK(state.commits == 0);
}
