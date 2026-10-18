add_library(cas.storage.fs)

target_sources(
    cas.storage.fs

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    concepts.cppm
    file_writer.cppm
    object_path.cppm
    storage_writer.cppm
    temp_path.cppm

    module.cppm
)

target_link_libraries(
    cas.storage.fs

    cas.hash
    cas.result
    cas.storage
)
