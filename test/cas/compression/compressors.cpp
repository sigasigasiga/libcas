#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <zlib.h>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

import cas.compression;

using cas::compression::compressor;

namespace {

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

// Feeds `data` to a fresh compressor in chunks of `chunk_size` bytes.
template<compressor C>
std::vector<std::byte> compress(std::span<std::byte const> data, std::size_t chunk_size) {
    C c;
    std::vector<std::byte> out;

    while (!data.empty()) {
        auto const n = std::min(chunk_size, data.size());
        c.feed(data.first(n), out);
        data = data.subspan(n);
    }

    std::move(c).finalize(out);
    return out;
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

} // namespace

TEMPLATE_TEST_CASE(
    "compressors satisfy the concept",
    "[compression]",
    cas::compression::none,
    cas::compression::zlib<>,
    cas::compression::zlib<0>,
    cas::compression::zlib<9>
) {
    STATIC_REQUIRE(compressor<TestType>);
}

TEST_CASE("none passes the data through", "[compression][none]") {
    SECTION("empty input") {
        CHECK(compress<cas::compression::none>({}, 1).empty());
    }

    SECTION("chunked input") {
        auto const data = make_bytes(1000);
        CHECK(compress<cas::compression::none>(data, 7) == data);
    }
}

TEMPLATE_TEST_CASE(
    "zlib produces a stream which zlib can decompress",
    "[compression][zlib]",
    cas::compression::zlib<>,
    cas::compression::zlib<0>,
    cas::compression::zlib<1>,
    cas::compression::zlib<9>
) {
    SECTION("empty input") {
        auto const compressed = compress<TestType>({}, 1);

        CHECK(!compressed.empty());
        CHECK(zlib_uncompress(compressed, 1).empty());
    }

    SECTION("short input") {
        auto const data = make_bytes("hello, world");
        auto const compressed = compress<TestType>(data, data.size());

        CHECK(zlib_uncompress(compressed, data.size()) == data);
    }

    SECTION("large input fed in different chunk sizes") {
        auto const data = make_bytes(1024 * 1024 + 123);

        for (std::size_t chunk_size : {std::size_t{1000}, std::size_t{64 * 1024}, data.size()}) {
            auto const compressed = compress<TestType>(data, chunk_size);
            CHECK(zlib_uncompress(compressed, data.size()) == data);
        }
    }
}

TEST_CASE("zlib actually compresses", "[compression][zlib]") {
    auto const data = std::vector<std::byte>(1024 * 1024, std::byte{'a'});
    auto const compressed = compress<cas::compression::zlib<>>(data, 64 * 1024);

    CHECK(compressed.size() < data.size() / 100);
    CHECK(zlib_uncompress(compressed, data.size()) == data);
}
