add_executable(cas.storage.fs.test.file_writer file_writer.cpp)
target_link_libraries(cas.storage.fs.test.file_writer PRIVATE cas.storage.fs Catch2::Catch2WithMain)

catch_discover_tests(cas.storage.fs.test.file_writer)

add_executable(cas.storage.fs.test.storage_writer storage_writer.cpp)
target_link_libraries(cas.storage.fs.test.storage_writer PRIVATE cas.storage.fs Catch2::Catch2WithMain)

catch_discover_tests(cas.storage.fs.test.storage_writer)
