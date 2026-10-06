module;

#include <openssl/evp.h>

export module cas.hash:constants;

namespace cas::hash {

export constexpr std::size_t max_raw_digest_size = EVP_MAX_MD_SIZE;
export constexpr std::size_t max_hex_digest_size = max_raw_digest_size * 2;

} // namespace cas::hash
