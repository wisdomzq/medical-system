set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Network Sql REQUIRED)

set(srcs
    main.cpp
    core/database/database.cpp
    core/network/src/server/communicationserver.cpp
    core/network/src/server/clienthandler.cpp
    core/network/src/server/messagerouter.cpp
    modules/loginmodule/loginmodule.cpp
)

add_executable(server
    ${srcs}
)

target_include_directories(server PRIVATE . core core/database core/network/src modules)
target_link_libraries(server PRIVATE Qt5::Core Qt5::Network Qt5::Sql sqlite3)