module;

#include <array>
#include <cassert>
#include <filesystem>
#include <string_view>

export module cas.storage.fs:make_object_path;

import cas.hash;
import cas.storage;

namespace cas::storage::fs {

// Objects are laid out the same way git lays out its loose objects:
// the first two hex digits of the digest name a directory,
// and the rest of them name the file inside it.
export std::filesystem::path make_object_path(std::filesystem::path const &root, object_handle const &handle) {
    assert(!handle.span().empty());

    std::array<char, hash::max_hex_digest_size> hex;
    auto const end = hash::to_hex(handle.span(), hex.begin());

    auto const name = std::string_view(hex.begin(), end);
    return root / name.substr(0, 2) / name.substr(2);
}

} // namespace cas::storage::fs
