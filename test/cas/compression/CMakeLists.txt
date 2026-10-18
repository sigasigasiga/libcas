add_executable(cas.compression.test.compressors compressors.cpp)
target_link_libraries(cas.compression.test.compressors PRIVATE cas.compression ZLIB::ZLIB Catch2::Catch2WithMain)

catch_discover_tests(cas.compression.test.compressors)
