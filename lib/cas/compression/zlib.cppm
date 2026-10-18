module;

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <vector>

#define ZLIB_CONST
#include <zlib.h>

export module cas.compression:zlib;

namespace cas::compression {

export constexpr int zlib_default_level = Z_DEFAULT_COMPRESSION;

namespace detail {

struct deflate_end
{
    void operator()(::z_stream *s) const noexcept {
        ::deflateEnd(s);
        delete s;
    }
};

// Keeps zlib out of the template, so that it is only ever compiled within this module.
class deflater
{
public:
    // `z_stream` must not be moved after initialization, hence the indirection
    explicit deflater(int level)
        : m_stream(new ::z_stream{})
    {
        int res = ::deflateInit(m_stream.get(), level);
        if (res == Z_MEM_ERROR) {
            throw std::bad_alloc();
        }

        if (res != Z_OK) {
            throw std::runtime_error(std::format("zlib: deflateInit failed: {}", ::zError(res)));
        }
    }

    deflater(deflater &&) = default;
    deflater &operator=(deflater &&) = default;
    ~deflater();

public:
    void feed(std::span<std::byte const> in, std::vector<std::byte> &out) {
        deflate(in, Z_NO_FLUSH, out);
    }

    void finalize(std::vector<std::byte> &out) {
        deflate({}, Z_FINISH, out);
    }

private:
    static constexpr std::size_t out_chunk_size = 16 * 1024;

    void deflate(std::span<std::byte const> in, int flush, std::vector<std::byte> &out) {
        // `avail_in` is `uInt`, so the input may have to be fed in pieces
        constexpr std::size_t max_piece_size = std::numeric_limits<::uInt>::max();

        [[maybe_unused]] int res;

        do {
            auto const piece = in.first(std::min(in.size(), max_piece_size));
            in = in.subspan(piece.size());

            m_stream->next_in = reinterpret_cast<::Bytef const *>(piece.data());
            m_stream->avail_in = static_cast<::uInt>(piece.size());

            do {
                auto const offset = out.size();
                out.resize(offset + out_chunk_size);

                m_stream->next_out = reinterpret_cast<::Bytef *>(out.data() + offset);
                m_stream->avail_out = out_chunk_size;

                res = ::deflate(m_stream.get(), in.empty() ? flush : Z_NO_FLUSH);
                assert(res != Z_STREAM_ERROR);

                out.resize(out.size() - m_stream->avail_out);
            } while (m_stream->avail_out == 0);

            assert(m_stream->avail_in == 0);
        } while (!in.empty());

        assert(flush != Z_FINISH || res == Z_STREAM_END);
    }

private:
    std::unique_ptr<::z_stream, deflate_end> m_stream;
};

deflater::~deflater() = default;

} // namespace detail

// Produces a zlib stream (RFC 1950), the same format git uses for its loose objects.
export template<int Level = zlib_default_level>
requires (Level == Z_DEFAULT_COMPRESSION || (Z_NO_COMPRESSION <= Level && Level <= Z_BEST_COMPRESSION))
class zlib
{
public:
    zlib()
        : m_deflater(Level)
    {
    }

public: // compressor
    void feed(std::span<std::byte const> in, std::vector<std::byte> &out) {
        m_deflater.feed(in, out);
    }

    void finalize(std::vector<std::byte> &out) && {
        m_deflater.finalize(out);
    }

private:
    detail::deflater m_deflater;
};

} // namespace cas::compression
