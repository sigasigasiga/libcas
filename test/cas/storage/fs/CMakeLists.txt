add_executable(cas.storage.fs.test.writer writer.cpp)
target_link_libraries(cas.storage.fs.test.writer PRIVATE cas.storage.fs ZLIB::ZLIB Catch2::Catch2WithMain)

catch_discover_tests(cas.storage.fs.test.writer)
