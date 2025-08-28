cmake_minimum_required(VERSION 3.5)
project(hello LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "/opt/Qt/5.15.2/gcc_64")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets Network Sql REQUIRED)

# Client application
aux_source_directory(./src srcs)
set(srcs ${srcs}
    src/doctorinfowidget.cpp
    src/patientinfowidget.cpp
    src/tcpclient.cpp
)
add_executable(hello
    WIN32
    ${srcs}
)
target_link_libraries(hello PRIVATE Qt5::Widgets Qt5::Network sqlite3)

# Server application
add_executable(server
    server/server.cpp
    server/tcpserver.cpp
    server/database.cpp
)
target_link_libraries(server PRIVATE Qt5::Core Qt5::Network Qt5::Sql sqlite3)
target_include_directories(server PRIVATE src)

# Specify MSVC UTF-8 encoding
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
