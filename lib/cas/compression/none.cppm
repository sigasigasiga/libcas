module;

#include <cstddef>
#include <span>
#include <vector>

export module cas.compression:none;

namespace cas::compression {

// Passes the data through as is.
export class none
{
public: // compressor
    void feed(std::span<std::byte const> in, std::vector<std::byte> &out) {
        out.insert(out.end(), in.begin(), in.end());
    }

    void finalize(std::vector<std::byte> &) && {}
};

} // namespace cas::compression
