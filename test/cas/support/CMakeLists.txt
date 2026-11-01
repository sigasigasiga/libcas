add_library(cas.test.support)

target_sources(
    cas.test.support

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    bytes.cppm
    read_file.cppm
    temp_dir.cppm

    module.cppm
)

target_link_libraries(
    cas.test.support

    PRIVATE

    cas.util
)
