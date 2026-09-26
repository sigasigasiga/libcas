cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(
    libcas

    LANGUAGES CXX
)

# Generate a .gitignore in the build directory to ignore all its contents
if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  file(WRITE "${CMAKE_BINARY_DIR}/.gitignore" "*\n")
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)
add_subdirectory(third_party/grace EXCLUDE_FROM_ALL)

add_subdirectory(lib)
