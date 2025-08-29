cmake_minimum_required(VERSION 3.16)

project(CommunicationModule VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COMM_MODULE_BUILD_EXAMPLES "Build client/server example executables" ON)

# Find Qt
find_package(Qt5 REQUIRED COMPONENTS Core Network)

# Library: CommunicationModule (static)
add_library(CommunicationModule STATIC
    # Sources
    src/client/communicationclient.cpp
    src/client/communicationclient.h
    src/client/streamparser.cpp
    src/client/streamparser.h
    src/client/responsedispatcher.cpp
    src/client/responsedispatcher.h

    src/protocol.h
)

target_include_directories(CommunicationModule
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(CommunicationModule PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(CommunicationModule PUBLIC Qt5::Core Qt5::Network)

# Examples (optional)
if (COMM_MODULE_BUILD_EXAMPLES)
    add_executable(ClientCLI
        examples/client_cli.cpp
    )
    target_link_libraries(ClientCLI PRIVATE CommunicationModule)
endif()

# Optional: simple install (headers + static lib)
install(TARGETS CommunicationModule
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)