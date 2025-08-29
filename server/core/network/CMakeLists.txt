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
    src/server/communicationserver.cpp
    src/server/clienthandler.cpp
    src/server/messagerouter.cpp
    src/server/communicationserver.h
    src/server/clienthandler.h
    src/server/messagerouter.h

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
    add_executable(ServerApp
        examples/server_main.cpp
    )
    target_link_libraries(ServerApp PRIVATE CommunicationModule)
endif()

# Optional: simple install (headers + static lib)
install(TARGETS CommunicationModule
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)