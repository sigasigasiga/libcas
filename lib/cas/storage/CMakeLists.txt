add_library(cas.storage)

target_sources(
    cas.storage

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    concepts.cppm
    object_handle.cppm
    put.cppm

    module.cppm
)

target_link_libraries(
    cas.storage

    cas.hash
    cas.reader
    cas.result
)
