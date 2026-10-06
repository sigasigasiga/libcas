#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

import cas.hash;

using cas::hash::from_hex;
using cas::hash::to_hex;

namespace {

constexpr std::array sample_bytes{
    std::byte{0x00},
    std::byte{0x01},
    std::byte{0x7f},
    std::byte{0x80},
    std::byte{0xab},
    std::byte{0xcd},
    std::byte{0xef},
    std::byte{0xff},
};

constexpr std::string_view sample_hex = "00017f80abcdefff";

std::string to_hex_string(std::span<std::byte const> digest) {
    std::string ret;
    to_hex(digest, std::back_inserter(ret));
    return ret;
}

std::vector<std::byte> from_hex_vector(std::string_view hex) {
    std::vector<std::byte> ret;
    from_hex(hex, std::back_inserter(ret));
    return ret;
}

consteval bool constexpr_roundtrip() {
    std::array<char, sample_hex.size()> hex{};
    std::array<std::byte, sample_bytes.size()> bytes{};

    if (to_hex(sample_bytes, hex.begin()) != hex.end()) {
        return false;
    }

    if (std::string_view(hex.data(), hex.size()) != sample_hex) {
        return false;
    }

    if (from_hex(std::string_view(hex.data(), hex.size()), bytes.begin()) != bytes.end()) {
        return false;
    }

    return bytes == sample_bytes;
}

} // namespace

TEST_CASE("to_hex encodes bytes as lowercase hex", "[hash][digest]") {
    SECTION("empty input produces empty output") {
        CHECK(to_hex_string({}).empty());
    }

    SECTION("single bytes") {
        CHECK(to_hex_string(std::array{std::byte{0x00}}) == "00");
        CHECK(to_hex_string(std::array{std::byte{0x0f}}) == "0f");
        CHECK(to_hex_string(std::array{std::byte{0xf0}}) == "f0");
        CHECK(to_hex_string(std::array{std::byte{0xff}}) == "ff");
    }

    SECTION("multiple bytes preserve order") {
        CHECK(to_hex_string(sample_bytes) == sample_hex);
    }

    SECTION("every byte value maps to two hex characters") {
        for (unsigned v = 0; v < 256; ++v) {
            auto const hex = to_hex_string(std::array{static_cast<std::byte>(v)});
            REQUIRE(hex.size() == 2);
            CHECK(hex == std::format("{:02x}", v));
        }
    }
}

TEST_CASE("to_hex returns the iterator past the last written character", "[hash][digest]") {
    std::array<char, 2 * sample_bytes.size() + 1> buf{};
    buf.back() = '!';

    auto const end = to_hex(sample_bytes, buf.begin());

    CHECK(end == buf.end() - 1);
    CHECK(std::string_view(buf.begin(), end) == sample_hex);
    CHECK(buf.back() == '!');
}

TEST_CASE("from_hex decodes hex into bytes", "[hash][digest]") {
    SECTION("empty input produces empty output") {
        CHECK(from_hex_vector("").empty());
    }

    SECTION("lowercase") {
        CHECK(from_hex_vector(sample_hex) == std::vector(sample_bytes.begin(), sample_bytes.end()));
    }

    SECTION("uppercase") {
        CHECK(from_hex_vector("00017F80ABCDEFFF") == std::vector(sample_bytes.begin(), sample_bytes.end()));
    }

    SECTION("mixed case") {
        CHECK(from_hex_vector("aB") == std::vector{std::byte{0xab}});
        CHECK(from_hex_vector("Ab") == std::vector{std::byte{0xab}});
    }

    SECTION("every byte value round-trips") {
        for (unsigned v = 0; v < 256; ++v) {
            auto const b = static_cast<std::byte>(v);
            CHECK(from_hex_vector(std::format("{:02x}", v)) == std::vector{b});
            CHECK(from_hex_vector(std::format("{:02X}", v)) == std::vector{b});
        }
    }
}

TEST_CASE("from_hex returns the iterator past the last written byte", "[hash][digest]") {
    std::array<std::byte, sample_bytes.size() + 1> buf{};
    buf.back() = std::byte{0x42};

    auto const end = from_hex(sample_hex, buf.begin());

    CHECK(end == buf.end() - 1);
    CHECK(std::equal(buf.begin(), end, sample_bytes.begin(), sample_bytes.end()));
    CHECK(buf.back() == std::byte{0x42});
}

TEST_CASE("from_hex rejects invalid input", "[hash][digest]") {
    SECTION("odd length") {
        CHECK_THROWS_AS(from_hex_vector("a"), std::invalid_argument);
        CHECK_THROWS_AS(from_hex_vector("abc"), std::invalid_argument);
    }

    SECTION("non-hex characters") {
        CHECK_THROWS_AS(from_hex_vector("g0"), std::invalid_argument);
        CHECK_THROWS_AS(from_hex_vector("0g"), std::invalid_argument);
        CHECK_THROWS_AS(from_hex_vector("G0"), std::invalid_argument);
        CHECK_THROWS_AS(from_hex_vector("  "), std::invalid_argument);
        CHECK_THROWS_AS(from_hex_vector("0x"), std::invalid_argument);
        CHECK_THROWS_AS(from_hex_vector(std::string_view("\0\0", 2)), std::invalid_argument);
    }

    SECTION("odd length is rejected before anything is written") {
        std::vector<std::byte> out;

        CHECK_THROWS_AS(from_hex("abc", std::back_inserter(out)), std::invalid_argument);
        CHECK(out.empty());
    }

    SECTION("bytes before an invalid character may be written") {
        std::vector<std::byte> out;

        CHECK_THROWS_AS(from_hex("abzz", std::back_inserter(out)), std::invalid_argument);
        CHECK((out.empty() || out == std::vector{std::byte{0xab}}));
    }
}

TEST_CASE("from_hex inverts to_hex", "[hash][digest]") {
    CHECK(from_hex_vector(to_hex_string(sample_bytes)) == std::vector(sample_bytes.begin(), sample_bytes.end()));
    CHECK(to_hex_string(from_hex_vector(sample_hex)) == sample_hex);
}

TEST_CASE("to_hex and from_hex are usable in constant expressions", "[hash][digest]") {
    STATIC_REQUIRE(constexpr_roundtrip());
}
