add_library(cas.hash)

target_sources(
    cas.hash

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    concepts.cppm
    sha256.cppm

    module.cppm
)

target_link_libraries(
    cas.hash

    PRIVATE

    grace.fn.bind
    OpenSSL::Crypto
)
