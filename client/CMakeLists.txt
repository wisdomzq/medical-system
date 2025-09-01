find_package(Qt5 COMPONENTS Widgets Network Sql REQUIRED)

add_executable(hello)

# Enable Qt code generation at target scope
set_target_properties(hello PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Use C++17 in a target-scoped and portable way
target_compile_features(hello PRIVATE cxx_std_17)

# Sources
target_sources(hello PRIVATE
    ui/hello/hello.cpp
    ui/loginwidget/loginwidget.cpp
    ui/doctorinfowidget/doctorinfowidget.cpp
    ui/doctorinfowidget/attendancewidget.cpp
    ui/doctorinfowidget/chatroomwidget.cpp
    ui/doctorinfowidget/profilewidget.cpp
    ui/doctorinfowidget/appointmentswidget.cpp
    ui/doctorinfowidget/appointmentdetailsdialog.cpp
    ui/doctorinfowidget/medicationselectiondialog.cpp
    ui/doctorinfowidget/datachartwidget.cpp
    ui/doctorinfowidget/remotedatawidget.cpp
    ui/patientinfowidget/patientinfowidget.cpp
    ui/common/chatbubbledelegate.cpp
    ui/common/chatbubbledelegate.h
    ui/patientinfowidget/communicationpage.cpp
    ui/patientinfowidget/communicationpage.h
    ui/patientinfowidget/basepage.h
    ui/patientinfowidget/appointmentpage.cpp
    ui/patientinfowidget/appointmentpage.h
    ui/patientinfowidget/profilepage.cpp
    ui/patientinfowidget/profilepage.h
    ui/patientinfowidget/casepage.cpp
    ui/patientinfowidget/casepage.h
    ui/patientinfowidget/placeholderpages.cpp
    ui/patientinfowidget/placeholderpages.h
    ui/patientinfowidget/hospitalpage.cpp
    ui/patientinfowidget/hospitalpage.h
    ui/patientinfowidget/medicationpage.cpp
    ui/patientinfowidget/medicationpage.h
    ui/patientinfowidget/evaluatepage.cpp
    ui/patientinfowidget/evaluatepage.h
    ui/patientinfowidget/qrcodedialog.cpp
    ui/patientinfowidget/qrcodedialog.h
    ui/patientinfowidget/paymentsuccessdialog.cpp
    ui/patientinfowidget/paymentsuccessdialog.h
    ui/patientinfowidget/prescriptionpage.cpp
    ui/patientinfowidget/prescriptionpage.h
    ui/patientinfowidget/advicepage.cpp
    ui/patientinfowidget/advicepage.h
    ui/patientinfowidget/doctorinfopage.cpp
    ui/patientinfowidget/doctorinfopage.h
    ui/patientinfowidget/doctorlistpage.cpp
    ui/patientinfowidget/doctorlistpage.h
    core/network/communicationclient.cpp
    core/network/responsedispatcher.cpp
    core/network/streamparser.cpp
    core/services/authservice.cpp
    core/services/patientservice.cpp
    core/services/appointmentservice.cpp
    core/services/doctorprofileservice.cpp
    core/services/patientappointmentservice.cpp
    core/services/adviceservice.cpp
    core/services/medicationservice.cpp
    core/services/evaluateservice.cpp
    core/services/doctorlistservice.cpp
    core/services/hospitalizationservice.cpp
    core/services/medicalrecordservice.cpp
    core/services/prescriptionservice.cpp
    core/services/attendanceservice.cpp
    core/services/medicalcrudservice.cpp
    core/services/chatservice.cpp
    resources/resources.qrc
    main.cpp
)

target_include_directories(hello PRIVATE
    .
    ui
    ui/hello
    ui/loginwidget
    ui/doctorinfowidget
    ui/patientinfowidget
    core
    core/network/
    core/services
    core/logging
)

target_link_libraries(hello PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
    project_warnings
)
