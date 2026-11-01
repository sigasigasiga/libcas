add_executable(cas.util.test.errno_error errno_error.cpp)
target_link_libraries(cas.util.test.errno_error PRIVATE cas.util Catch2::Catch2WithMain)

catch_discover_tests(cas.util.test.errno_error)

add_executable(cas.util.test.posix posix.cpp)
target_link_libraries(cas.util.test.posix PRIVATE cas.util Catch2::Catch2WithMain)

catch_discover_tests(cas.util.test.posix)
