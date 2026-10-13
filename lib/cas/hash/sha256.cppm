module;

#include <cassert>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

export module cas.hash:sha256;

import grace.fn.bind;

namespace cas::hash {

// Drains the thread's OpenSSL error queue so stale errors don't leak into
// the next failure report.
[[noreturn]] void throw_openssl_error(char const *what) {
    std::string msg = what;
    char buf[256];
    bool first = true;

    while (unsigned long err = ::ERR_get_error()) {
        ::ERR_error_string_n(err, buf, sizeof(buf));
        msg += first ? ": " : "; ";
        msg += buf;
        first = false;
    }

    if (first) {
        msg += ": no error reported by OpenSSL";
    }

    throw std::runtime_error(msg);
}

export class sha256
{
public:
    sha256()
        : m_ctx(::EVP_MD_CTX_new())
    {
        if (!m_ctx) {
            throw std::bad_alloc();
        }

        if (::EVP_DigestInit_ex(m_ctx.get(), ::EVP_sha256(), nullptr) != 1) {
            throw_openssl_error("sha256: EVP_DigestInit_ex failed");
        }
    }

    static constexpr std::size_t digest_size() noexcept {
        return SHA256_DIGEST_LENGTH;
    }

    void feed(std::span<std::byte const> in) {
        if (in.empty()) {
            return;
        }

        if (::EVP_DigestUpdate(m_ctx.get(), in.data(), in.size()) != 1) {
            throw_openssl_error("sha256: EVP_DigestUpdate failed");
        }
    }

    void finalize(std::span<std::byte> out) {
        assert(out.size() >= digest_size());

        unsigned int len = 0;

        int res = ::EVP_DigestFinal_ex(
            m_ctx.get(),
            reinterpret_cast<unsigned char *>(out.data()),
            &len
        );

        if (res != 1) {
            throw_openssl_error("sha256: EVP_DigestFinal_ex failed");
        }

        assert(len == digest_size());
    }

private:
    std::unique_ptr<EVP_MD_CTX, grace::fn::bind::lift_invocable<::EVP_MD_CTX_free>> m_ctx;
};

} // namespace cas::hash
