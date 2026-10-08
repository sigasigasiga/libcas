module;

#include <concepts>
#include <cstddef>
#include <iterator>
#include <span>
#include <utility>

export module cas.reader:iterator_reader;

namespace cas::reader {

export template<std::input_iterator I, std::sentinel_for<I> S>
requires std::convertible_to<std::iter_reference_t<I>, std::byte>
class iterator_reader
{
public:
    iterator_reader(I it, S sent)
        : m_it(std::move(it))
        , m_sent(std::move(sent))
    {
    }

public: // reader
    std::size_t read(std::span<std::byte> out) {
        std::size_t n = 0;
        for (; n < out.size() && m_it != m_sent; ++n, ++m_it) {
            out[n] = *m_it;
        }
        return n;
    }

private:
    I m_it;
    S m_sent;
};

} // namespace cas::reader
