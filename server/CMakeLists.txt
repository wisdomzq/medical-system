find_package(Qt5 COMPONENTS Core Network Sql REQUIRED)

add_executable(server)

set_target_properties(server PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_compile_features(server PRIVATE cxx_std_17)

target_sources(server PRIVATE
    main.cpp
    core/database/database.cpp
    core/network/communicationserver.cpp
    core/network/clienthandler.cpp
    core/network/filetransferprocessor.cpp
    core/network/streamparser.cpp
    core/network/messagerouter.cpp
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
    modules/chatmodule/chatmodule.cpp
)

target_include_directories(server PRIVATE
    .
    core
    core/database
    core/network
    modules
)

target_link_libraries(server PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    project_warnings
)