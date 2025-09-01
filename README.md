
```
test
├─ APPOINTMENT_ENHANCEMENT.md
├─ CMakeLists.txt
├─ PRESCRIPTION_IMPLEMENTATION_SUMMARY.md
├─ README.md
├─ client
│  ├─ CMakeLists.txt
│  ├─ core
│  │  ├─ logging
│  │  │  └─ logging.h
│  │  ├─ network
│  │  │  ├─ communicationclient.cpp
│  │  │  ├─ communicationclient.h
│  │  │  ├─ protocol.h
│  │  │  ├─ responsedispatcher.cpp
│  │  │  ├─ responsedispatcher.h
│  │  │  ├─ streamparser.cpp
│  │  │  └─ streamparser.h
│  │  └─ services
│  │     ├─ adviceservice.cpp
│  │     ├─ adviceservice.h
│  │     ├─ appointmentservice.cpp
│  │     ├─ appointmentservice.h
│  │     ├─ attendanceservice.cpp
│  │     ├─ attendanceservice.h
│  │     ├─ authservice.cpp
│  │     ├─ authservice.h
│  │     ├─ chatservice.cpp
│  │     ├─ chatservice.h
│  │     ├─ doctorlistservice.cpp
│  │     ├─ doctorlistservice.h
│  │     ├─ doctorprofileservice.cpp
│  │     ├─ doctorprofileservice.h
│  │     ├─ evaluateservice.cpp
│  │     ├─ evaluateservice.h
│  │     ├─ hospitalizationservice.cpp
│  │     ├─ hospitalizationservice.h
│  │     ├─ medicalcrudservice.cpp
│  │     ├─ medicalcrudservice.h
│  │     ├─ medicalrecordservice.cpp
│  │     ├─ medicalrecordservice.h
│  │     ├─ medicationservice.cpp
│  │     ├─ medicationservice.h
│  │     ├─ patientappointmentservice.cpp
│  │     ├─ patientappointmentservice.h
│  │     ├─ patientservice.cpp
│  │     ├─ patientservice.h
│  │     ├─ prescriptionservice.cpp
│  │     └─ prescriptionservice.h
│  ├─ main.cpp
│  ├─ resources
│  │  ├─ background.jpeg
│  │  ├─ doctor_attendance.qss
│  │  ├─ doctorinfo.qss
│  │  ├─ icons
│  │  │  ├─ 住院信息.svg
│  │  │  ├─ 医嘱.svg
│  │  │  ├─ 医患沟通.svg
│  │  │  ├─ 医生.svg
│  │  │  ├─ 地址.svg
│  │  │  ├─ 基本信息.svg
│  │  │  ├─ 处方.svg
│  │  │  ├─ 密码.svg
│  │  │  ├─ 年龄.svg
│  │  │  ├─ 我的病例.svg
│  │  │  ├─ 我的预约.svg
│  │  │  ├─ 打卡.svg
│  │  │  ├─ 数据图表.svg
│  │  │  ├─ 智慧医疗系统.svg
│  │  │  ├─ 查看医生信息.svg
│  │  │  ├─ 用户名.svg
│  │  │  ├─ 电话.svg
│  │  │  ├─ 病人.svg
│  │  │  ├─ 科室.svg
│  │  │  ├─ 考勤.svg
│  │  │  ├─ 聊天室.svg
│  │  │  ├─ 药品搜索.svg
│  │  │  ├─ 评估与充值.svg
│  │  │  ├─ 请假.svg
│  │  │  ├─ 返回.svg
│  │  │  ├─ 远程数据采集.svg
│  │  │  ├─ 退出登录.svg
│  │  │  ├─ 销假.svg
│  │  │  └─ 预约信息.svg
│  │  ├─ login.qss
│  │  ├─ patientinfo.qss
│  │  ├─ resources.qrc
│  │  ├─ style.qss
│  │  └─ 智慧医疗系统.png
│  └─ ui
│     ├─ authdialog
│     │  ├─ authdialog.cpp
│     │  └─ authdialog.h
│     ├─ common
│     │  ├─ chatbubbledelegate.cpp
│     │  └─ chatbubbledelegate.h
│     ├─ doctorinfowidget
│     │  ├─ appointmentdetailsdialog.cpp
│     │  ├─ appointmentdetailsdialog.h
│     │  ├─ appointmentswidget.cpp
│     │  ├─ appointmentswidget.h
│     │  ├─ attendancewidget.cpp
│     │  ├─ attendancewidget.h
│     │  ├─ cancelleavewidget.cpp
│     │  ├─ cancelleavewidget.h
│     │  ├─ caseswidget.cpp
│     │  ├─ caseswidget.h
│     │  ├─ chatroomwidget.cpp
│     │  ├─ chatroomwidget.h
│     │  ├─ checkinwidget.cpp
│     │  ├─ checkinwidget.h
│     │  ├─ datachartwidget.cpp
│     │  ├─ datachartwidget.h
│     │  ├─ diagnosiswidget.cpp
│     │  ├─ diagnosiswidget.h
│     │  ├─ doctorinfowidget.cpp
│     │  ├─ doctorinfowidget.h
│     │  ├─ leavewidget.cpp
│     │  ├─ leavewidget.h
│     │  ├─ medicationselectiondialog.cpp
│     │  ├─ medicationselectiondialog.h
│     │  ├─ profilewidget.cpp
│     │  ├─ profilewidget.h
│     │  ├─ remotedatawidget.cpp
│     │  └─ remotedatawidget.h
│     ├─ loginwidget
│     │  ├─ loginwidget.cpp
│     │  └─ loginwidget.h
│     └─ patientinfowidget
│        ├─ advicepage.cpp
│        ├─ advicepage.h
│        ├─ appointmentpage.cpp
│        ├─ appointmentpage.h
│        ├─ basepage.h
│        ├─ casepage.cpp
│        ├─ casepage.h
│        ├─ communicationpage.cpp
│        ├─ communicationpage.h
│        ├─ doctorinfopage.cpp
│        ├─ doctorinfopage.h
│        ├─ doctorlistpage.cpp
│        ├─ doctorlistpage.h
│        ├─ evaluatepage.cpp
│        ├─ evaluatepage.h
│        ├─ hospitalpage.cpp
│        ├─ hospitalpage.h
│        ├─ medicationpage.cpp
│        ├─ medicationpage.h
│        ├─ patientinfowidget.cpp
│        ├─ patientinfowidget.h
│        ├─ paymentsuccessdialog.cpp
│        ├─ paymentsuccessdialog.h
│        ├─ placeholderpages.cpp
│        ├─ placeholderpages.h
│        ├─ prescriptionpage.cpp
│        ├─ prescriptionpage.h
│        ├─ profilepage.cpp
│        ├─ profilepage.h
│        ├─ qrcodedialog.cpp
│        └─ qrcodedialog.h
├─ demo_prescription.py
├─ pexels-urfriendlyphotog-26547171.jpg
├─ readme.md
├─ resources
│  ├─ medications
│  │  ├─ README.md
│  │  ├─ images
│  │  │  ├─ .keep
│  │  │  ├─ acetaminophen.png
│  │  │  ├─ acetaminophen.txt
│  │  │  ├─ aminophylline.png
│  │  │  ├─ aminophylline.txt
│  │  │  ├─ amoxicillin.png
│  │  │  ├─ amoxicillin.txt
│  │  │  ├─ aspirin.png
│  │  │  ├─ aspirin.txt
│  │  │  ├─ aspirin_cardio.png
│  │  │  ├─ aspirin_cardio.txt
│  │  │  ├─ banlangen.png
│  │  │  ├─ banlangen.txt
│  │  │  ├─ calcium.png
│  │  │  ├─ calcium.txt
│  │  │  ├─ captopril.png
│  │  │  ├─ captopril.txt
│  │  │  ├─ carbamazepine.png
│  │  │  ├─ carbamazepine.txt
│  │  │  ├─ cefalexin.png
│  │  │  ├─ cefalexin.txt
│  │  │  ├─ compound_paracetamol.png
│  │  │  ├─ compound_paracetamol.txt
│  │  │  ├─ domperidone.png
│  │  │  ├─ domperidone.txt
│  │  │  ├─ erythromycin_ointment.png
│  │  │  ├─ erythromycin_ointment.txt
│  │  │  ├─ ganmaoling.png
│  │  │  ├─ ganmaoling.txt
│  │  │  ├─ gliclazide.png
│  │  │  ├─ gliclazide.txt
│  │  │  ├─ glucose_strips.png
│  │  │  ├─ glucose_strips.txt
│  │  │  ├─ huoxiang_zhengqi.png
│  │  │  ├─ huoxiang_zhengqi.txt
│  │  │  ├─ ibuprofen.png
│  │  │  ├─ ibuprofen.txt
│  │  │  ├─ levofloxacin.png
│  │  │  ├─ levofloxacin.txt
│  │  │  ├─ medical_mask.png
│  │  │  ├─ medical_mask.txt
│  │  │  ├─ metformin.png
│  │  │  ├─ metformin.txt
│  │  │  ├─ nifedipine.png
│  │  │  ├─ nifedipine.txt
│  │  │  ├─ omeprazole.png
│  │  │  ├─ omeprazole.txt
│  │  │  ├─ phenytoin.png
│  │  │  ├─ phenytoin.txt
│  │  │  ├─ salbutamol.png
│  │  │  ├─ salbutamol.txt
│  │  │  ├─ thermometer.png
│  │  │  ├─ thermometer.txt
│  │  │  ├─ vitamin_b.png
│  │  │  ├─ vitamin_b.txt
│  │  │  ├─ vitamin_c.png
│  │  │  ├─ vitamin_c.txt
│  │  │  ├─ yunnan_baiyao.png
│  │  │  └─ yunnan_baiyao.txt
│  │  └─ medications.json
│  └─ style.qss
├─ server
│  ├─ CMakeLists.txt
│  ├─ core
│  │  ├─ database
│  │  │  ├─ CMakeLists.txt
│  │  │  ├─ database.cpp
│  │  │  ├─ database.h
│  │  │  ├─ database_config.h
│  │  │  ├─ dbterminal.cpp.deprecated
│  │  │  └─ dbterminal.h
│  │  ├─ logging
│  │  │  └─ logging.h
│  │  └─ network
│  │     ├─ README.md
│  │     ├─ clienthandler.cpp
│  │     ├─ clienthandler.h
│  │     ├─ communicationserver.cpp
│  │     ├─ communicationserver.h
│  │     ├─ messagerouter.cpp
│  │     ├─ messagerouter.h
│  │     └─ protocol.h
│  ├─ main.cpp
│  ├─ modules
│  │  ├─ chatmodule
│  │  │  ├─ README.md
│  │  │  ├─ chatmodule.cpp
│  │  │  └─ chatmodule.h
│  │  ├─ doctormodule
│  │  │  ├─ assignment
│  │  │  │  ├─ assignment.cpp
│  │  │  │  └─ assignment.h
│  │  │  ├─ attendance
│  │  │  │  ├─ attendance.cpp
│  │  │  │  └─ attendance.h
│  │  │  ├─ profile
│  │  │  │  ├─ profile.cpp
│  │  │  │  └─ profile.h
│  │  │  └─ router
│  │  │     ├─ router.cpp
│  │  │     └─ router.h
│  │  ├─ loginmodule
│  │  │  ├─ loginmodule.cpp
│  │  │  ├─ loginmodule.h
│  │  │  ├─ loginrouter.cpp
│  │  │  └─ loginrouter.h
│  │  ├─ medicalcrud
│  │  │  ├─ medicalcrud.cpp
│  │  │  └─ medicalcrud.h
│  │  └─ patientmodule
│  │     ├─ advice
│  │     │  ├─ advice.cpp
│  │     │  └─ advice.h
│  │     ├─ appointment
│  │     │  ├─ appointment.cpp
│  │     │  └─ appointment.h
│  │     ├─ doctorinfo
│  │     │  ├─ doctorinfo.cpp
│  │     │  └─ doctorinfo.h
│  │     ├─ doctorlist
│  │     │  ├─ doctorlist.cpp
│  │     │  └─ doctorlist.h
│  │     ├─ evaluate
│  │     │  ├─ evaluate.cpp
│  │     │  └─ evaluate.h
│  │     ├─ hospital
│  │     │  ├─ hospital.cpp
│  │     │  └─ hospital.h
│  │     ├─ hospitalization
│  │     │  ├─ hospitalization.cpp
│  │     │  └─ hospitalization.h
│  │     ├─ medicalrecord
│  │     │  ├─ medicalrecord.cpp
│  │     │  └─ medicalrecord.h
│  │     ├─ medicine
│  │     │  ├─ img
│  │     │  │  ├─ 阿司匹林.jpg
│  │     │  │  └─ 阿司匹林.jpg:Zone.Identifier
│  │     │  ├─ medicine.cpp
│  │     │  └─ medicine.h
│  │     ├─ patientinfo
│  │     │  ├─ patientinfo.cpp
│  │     │  └─ patientinfo.h
│  │     ├─ prescription
│  │     │  ├─ prescription.cpp
│  │     │  └─ prescription.h
│  │     └─ register
│  │        ├─ register.cpp
│  │        └─ register.h
│  └─ server
├─ test
│  ├─ test_appointment_booking.py
│  ├─ test_create_appointment.cpp
│  ├─ test_db.cpp
│  ├─ test_hospitalization.py
│  ├─ test_patient_register.py
│  ├─ test_register.py
│  ├─ test_server.sh
│  └─ test_single_hospitalization.py
└─ test_prescription_workflow.py

```