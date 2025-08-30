cmake_minimum_required(VERSION 3.15)
project(hello LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "/opt/Qt/5.15.2/gcc_64")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(client)
add_subdirectory(server)

# Specify MSVC UTF-8 encoding
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")