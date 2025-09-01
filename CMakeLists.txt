cmake_minimum_required(VERSION 3.20)

# Avoid hyphen in project name for CMake target namespace
project(medical_system VERSION 0.1.0 LANGUAGES CXX)

# Optional: export compile commands for tooling (clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Centralized, target-scoped common options (example: MSVC UTF-8)
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
	$<$<CXX_COMPILER_ID:MSVC>:/utf-8>
	$<$<C_COMPILER_ID:MSVC>:/utf-8>
)

add_subdirectory(client)
add_subdirectory(server)