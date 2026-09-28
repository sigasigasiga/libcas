module;

#include <openssl/evp.h>

export module cas.hash:constants;

namespace cas::hash {

export constexpr std::size_t max_digest_size = EVP_MAX_MD_SIZE;

} // namespace cas::hash
