add_library(cas.storage.fs)

target_sources(
    cas.storage.fs

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    object_path.cppm
    temp_file.cppm
    writer.cppm

    module.cppm
)

target_link_libraries(
    cas.storage.fs

    cas.compression
    cas.hash
    cas.result
    cas.storage
)
