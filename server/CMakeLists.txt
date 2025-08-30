set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Network Sql REQUIRED)

set(srcs
    main.cpp
    core/database/database.cpp
    # 如果你的数据库终端功能也想保留，可以添加：
    # core/database/dbterminal.cpp  # 文件不存在，暂时注释掉
    core/network/src/server/communicationserver.cpp
    core/network/src/server/clienthandler.cpp
    core/network/src/server/messagerouter.cpp
    modules/loginmodule/loginmodule.cpp
    modules/loginmodule/loginrouter.cpp
    modules/patientmodule/register/register.cpp
    modules/patientmodule/medicine/medicine.cpp
    modules/patientmodule/evaluate/evaluate.cpp
    modules/patientmodule/prescription/prescription.cpp
    modules/patientmodule/advice/advice.cpp
    modules/patientmodule/doctorinfo/doctorinfo.cpp
    modules/patientmodule/medicalrecord/medicalrecord.cpp
    modules/patientmodule/doctorlist/doctorlist.cpp
    modules/patientmodule/patientinfo/patientinfo.cpp
    modules/patientmodule/appointment/appointment.cpp
    modules/patientmodule/hospitalization/hospitalization.cpp
    modules/medicalcrud/medicalcrud.cpp
    modules/doctormodule/profile/profile.cpp
    modules/doctormodule/assignment/assignment.cpp
    modules/doctormodule/attendance/attendance.cpp
    modules/doctormodule/router/router.cpp
)

add_executable(server
    ${srcs}
)

target_include_directories(server PRIVATE . core core/database core/network/src modules)
target_link_libraries(server PRIVATE Qt5::Core Qt5::Network Qt5::Sql)