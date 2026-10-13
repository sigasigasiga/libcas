add_library(cas.result)

target_sources(
    cas.result

    PUBLIC

    FILE_SET
    CXX_MODULES FILES

    result.cppm

    module.cppm
)

target_link_libraries(
    cas.result

    PRIVATE

    grace.exception
)
