/****************************************************************************
** Meta object code from reading C++ file 'loginwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../ui/loginwidget/loginwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'loginwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LoginWidget_t {
    QByteArrayData data[17];
    char stringdata0[252];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LoginWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LoginWidget_t qt_meta_stringdata_LoginWidget = {
    {
QT_MOC_LITERAL(0, 0, 11), // "LoginWidget"
QT_MOC_LITERAL(1, 12, 14), // "doctorLoggedIn"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 10), // "doctorName"
QT_MOC_LITERAL(4, 39, 15), // "patientLoggedIn"
QT_MOC_LITERAL(5, 55, 11), // "patientName"
QT_MOC_LITERAL(6, 67, 20), // "showInitialSelection"
QT_MOC_LITERAL(7, 88, 15), // "showDoctorLogin"
QT_MOC_LITERAL(8, 104, 16), // "showPatientLogin"
QT_MOC_LITERAL(9, 121, 18), // "showDoctorRegister"
QT_MOC_LITERAL(10, 140, 19), // "showPatientRegister"
QT_MOC_LITERAL(11, 160, 13), // "onDoctorLogin"
QT_MOC_LITERAL(12, 174, 14), // "onPatientLogin"
QT_MOC_LITERAL(13, 189, 16), // "onDoctorRegister"
QT_MOC_LITERAL(14, 206, 17), // "onPatientRegister"
QT_MOC_LITERAL(15, 224, 18), // "onResponseReceived"
QT_MOC_LITERAL(16, 243, 8) // "response"

    },
    "LoginWidget\0doctorLoggedIn\0\0doctorName\0"
    "patientLoggedIn\0patientName\0"
    "showInitialSelection\0showDoctorLogin\0"
    "showPatientLogin\0showDoctorRegister\0"
    "showPatientRegister\0onDoctorLogin\0"
    "onPatientLogin\0onDoctorRegister\0"
    "onPatientRegister\0onResponseReceived\0"
    "response"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LoginWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   74,    2, 0x06 /* Public */,
       4,    1,   77,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   80,    2, 0x08 /* Private */,
       7,    0,   81,    2, 0x08 /* Private */,
       8,    0,   82,    2, 0x08 /* Private */,
       9,    0,   83,    2, 0x08 /* Private */,
      10,    0,   84,    2, 0x08 /* Private */,
      11,    0,   85,    2, 0x08 /* Private */,
      12,    0,   86,    2, 0x08 /* Private */,
      13,    0,   87,    2, 0x08 /* Private */,
      14,    0,   88,    2, 0x08 /* Private */,
      15,    1,   89,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,   16,

       0        // eod
};

void LoginWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LoginWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->doctorLoggedIn((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->patientLoggedIn((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->showInitialSelection(); break;
        case 3: _t->showDoctorLogin(); break;
        case 4: _t->showPatientLogin(); break;
        case 5: _t->showDoctorRegister(); break;
        case 6: _t->showPatientRegister(); break;
        case 7: _t->onDoctorLogin(); break;
        case 8: _t->onPatientLogin(); break;
        case 9: _t->onDoctorRegister(); break;
        case 10: _t->onPatientRegister(); break;
        case 11: _t->onResponseReceived((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LoginWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoginWidget::doctorLoggedIn)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LoginWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoginWidget::patientLoggedIn)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject LoginWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_LoginWidget.data,
    qt_meta_data_LoginWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *LoginWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LoginWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LoginWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int LoginWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void LoginWidget::doctorLoggedIn(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LoginWidget::patientLoggedIn(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
