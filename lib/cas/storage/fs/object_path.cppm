module;

#include <cassert>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>

export module cas.storage.fs:object_path;

import cas.hash;
import cas.storage;

namespace cas::storage::fs {

// Objects are laid out the same way git lays out its loose objects:
// the first two hex digits of the digest name a directory,
// and the rest of them name the file inside it.
export std::filesystem::path object_path(std::filesystem::path const &root, object_handle const &handle) {
    assert(!handle.span().empty());

    std::string hex;
    hash::to_hex(handle.span(), std::back_inserter(hex));

    auto const name = std::string_view(hex);
    return root / name.substr(0, 2) / name.substr(2);
}

} // namespace cas::storage::fs
