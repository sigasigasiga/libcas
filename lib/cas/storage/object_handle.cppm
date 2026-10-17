module;

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <span>

export module cas.storage:object_handle;

import cas.hash;

namespace cas::storage {

export class object_handle
{
public:
    explicit object_handle(std::span<std::byte const> digest)
    {
        assert(digest.size() <= m_digest.size());
        m_digest_size = digest.size();
        std::ranges::copy(digest, m_digest.begin());
    }

public:
    auto span() const { return std::span(m_digest.cbegin(), m_digest_size); }

private:
    std::size_t m_digest_size;
    std::array<std::byte, hash::max_raw_digest_size> m_digest;
};

} // namespace cas::storage
