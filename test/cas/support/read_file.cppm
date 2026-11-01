module;

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <vector>

export module cas.test.support:read_file;

namespace cas::test::support {

export std::vector<std::byte> read_file(std::filesystem::path const &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("read_file: cannot open " + path.string());
    }

    // `views::istream` uses `operator>>`, which would skip whitespace otherwise
    in.unsetf(std::ios::skipws);

    std::vector<std::byte> ret;
    std::ranges::copy(
        std::views::istream<char>(in) | std::views::transform([](char c) { return std::byte(c); }),
        std::back_inserter(ret)
    );
    return ret;
}

} // namespace cas::test::support
