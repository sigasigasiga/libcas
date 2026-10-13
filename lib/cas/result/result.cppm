module;

#include <expected>

export module cas.result:result;

import grace.exception;

namespace cas::result {

export template<typename T, typename E = std::exception>
using result = std::expected<T, grace::exception::storage<E>>;

} // namespace cas::result
