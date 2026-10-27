add_executable(cas.storage.test.put put.cpp)
target_link_libraries(cas.storage.test.put PRIVATE cas.storage Catch2::Catch2WithMain)

catch_discover_tests(cas.storage.test.put)

add_subdirectory(fs)
