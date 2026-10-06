add_executable(cas.hash.test.digest digest.cpp)
target_link_libraries(cas.hash.test.digest PRIVATE cas.hash Catch2::Catch2WithMain)

catch_discover_tests(cas.hash.test.digest)
