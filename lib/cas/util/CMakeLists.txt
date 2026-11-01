add_library(cas.util)

target_sources(
    cas.util

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    errno_error.cppm
    posix.cppm
    unique_file.cppm

    module.cppm
)
