set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Network Sql REQUIRED)

set(srcs
    ui/hello/hello.cpp
    ui/loginwidget/loginwidget.cpp
    ui/doctorinfowidget/doctorinfowidget.cpp
    ui/patientinfowidget/patientinfowidget.cpp
    ui/patientinfowidget/basepage.h
    ui/patientinfowidget/appointmentpage.cpp
    ui/patientinfowidget/appointmentpage.h
    ui/patientinfowidget/profilepage.cpp
    ui/patientinfowidget/profilepage.h
    ui/patientinfowidget/placeholderpages.cpp
    ui/patientinfowidget/placeholderpages.h
    ui/patientinfowidget/hospitalpage.cpp
    ui/patientinfowidget/hospitalpage.h
    core/network/src/client/communicationclient.cpp
    core/network/src/client/responsedispatcher.cpp
    core/network/src/client/streamparser.cpp
    resources/resources.qrc
    main.cpp
)

add_executable(hello
    ${srcs}
)

target_include_directories(hello PRIVATE
    .
    ui
    ui/hello
    ui/loginwidget
    ui/doctorinfowidget
    ui/patientinfowidget
    core
    core/network/src
    core/network/src/client
)

target_link_libraries(hello PRIVATE Qt5::Widgets Qt5::Network sqlite3)