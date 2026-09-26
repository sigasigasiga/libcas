module;

#include <memory>
#include <span>

#include <openssl/evp.h>
#include <openssl/sha.h>

export module cas.hash:sha256;

import grace.fn.bind;

namespace cas::hash {

export class sha256
{
public:
    static constexpr std::size_t output_size = SHA256_DIGEST_LENGTH; // 32

    sha256()
        : m_ctx(::EVP_MD_CTX_new())
    {
        if (!m_ctx) {
            throw std::runtime_error("sha256: EVP_MD_CTX_new failed");
        }

        if (::EVP_DigestInit_ex(m_ctx.get(), ::EVP_sha256(), nullptr) != 1) {
            throw std::runtime_error("sha256: EVP_DigestInit_ex failed");
        }
    }

    static constexpr std::size_t digest_size() noexcept {
        return output_size;
    }

    void feed(std::span<std::byte const> in) {
        if (in.empty()) {
            return;
        }
        if (::EVP_DigestUpdate(m_ctx.get(), in.data(), in.size()) != 1) {
            throw std::runtime_error("sha256: EVP_DigestUpdate failed");
        }
    }

    void finalize(std::span<std::byte, output_size> out) {
        unsigned int len = 0;

        int res = ::EVP_DigestFinal_ex(
            m_ctx.get(),
            reinterpret_cast<unsigned char *>(out.data()),
            &len
        );

        if (res != 1) {
            throw std::runtime_error("sha256: EVP_DigestFinal_ex failed");
        }

        if (len != output_size) {
            throw std::runtime_error("sha256: unexpected digest length");
        }
    }

private:
    std::unique_ptr<EVP_MD_CTX, grace::fn::bind::lift_invocable<::EVP_MD_CTX_free>> m_ctx;
};

} // namespace cas::hash
