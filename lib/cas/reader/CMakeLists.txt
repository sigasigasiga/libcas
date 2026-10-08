add_library(cas.reader)

target_sources(
    cas.reader

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    concepts.cppm
    iterator_reader.cppm

    module.cppm
)
