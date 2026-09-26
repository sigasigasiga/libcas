add_library(cas.hash)

target_sources(
    cas.hash

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    concepts.cppm

    module.cppm
)

target_link_libraries(
    cas.hash

    PRIVATE

    #OpenSSL::Crypto
)
