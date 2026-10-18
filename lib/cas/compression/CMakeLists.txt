add_library(cas.compression)

target_sources(
    cas.compression

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    concepts.cppm
    none.cppm
    zlib.cppm

    module.cppm
)

target_link_libraries(
    cas.compression

    PRIVATE

    ZLIB::ZLIB
)
