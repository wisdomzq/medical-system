/****************************************************************************
** Meta object code from reading C++ file 'messagerouter.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/network/src/server/messagerouter.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'messagerouter.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MessageRouter_t {
    QByteArrayData data[14];
    char stringdata0[167];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MessageRouter_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MessageRouter_t qt_meta_stringdata_MessageRouter = {
    {
QT_MOC_LITERAL(0, 0, 13), // "MessageRouter"
QT_MOC_LITERAL(1, 14, 13), // "responseReady"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 14), // "ClientHandler*"
QT_MOC_LITERAL(4, 44, 6), // "target"
QT_MOC_LITERAL(5, 51, 21), // "Protocol::MessageType"
QT_MOC_LITERAL(6, 73, 4), // "type"
QT_MOC_LITERAL(7, 78, 7), // "payload"
QT_MOC_LITERAL(8, 86, 15), // "requestReceived"
QT_MOC_LITERAL(9, 102, 14), // "onRequestReady"
QT_MOC_LITERAL(10, 117, 6), // "sender"
QT_MOC_LITERAL(11, 124, 16), // "Protocol::Header"
QT_MOC_LITERAL(12, 141, 6), // "header"
QT_MOC_LITERAL(13, 148, 18) // "onBusinessResponse"

    },
    "MessageRouter\0responseReady\0\0"
    "ClientHandler*\0target\0Protocol::MessageType\0"
    "type\0payload\0requestReceived\0"
    "onRequestReady\0sender\0Protocol::Header\0"
    "header\0onBusinessResponse"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MessageRouter[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   34,    2, 0x06 /* Public */,
       8,    1,   41,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    3,   44,    2, 0x0a /* Public */,
      13,    2,   51,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5, QMetaType::QJsonObject,    4,    6,    7,
    QMetaType::Void, QMetaType::QJsonObject,    7,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 11, QMetaType::QJsonObject,   10,   12,    7,
    QMetaType::Void, 0x80000000 | 5, QMetaType::QJsonObject,    6,    7,

       0        // eod
};

void MessageRouter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MessageRouter *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->responseReady((*reinterpret_cast< ClientHandler*(*)>(_a[1])),(*reinterpret_cast< Protocol::MessageType(*)>(_a[2])),(*reinterpret_cast< QJsonObject(*)>(_a[3]))); break;
        case 1: _t->requestReceived((*reinterpret_cast< QJsonObject(*)>(_a[1]))); break;
        case 2: _t->onRequestReady((*reinterpret_cast< ClientHandler*(*)>(_a[1])),(*reinterpret_cast< Protocol::Header(*)>(_a[2])),(*reinterpret_cast< QJsonObject(*)>(_a[3]))); break;
        case 3: _t->onBusinessResponse((*reinterpret_cast< Protocol::MessageType(*)>(_a[1])),(*reinterpret_cast< QJsonObject(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Protocol::MessageType >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Protocol::Header >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Protocol::MessageType >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MessageRouter::*)(ClientHandler * , Protocol::MessageType , QJsonObject );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageRouter::responseReady)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MessageRouter::*)(QJsonObject );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageRouter::requestReceived)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MessageRouter::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MessageRouter.data,
    qt_meta_data_MessageRouter,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MessageRouter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MessageRouter::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MessageRouter.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MessageRouter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void MessageRouter::responseReady(ClientHandler * _t1, Protocol::MessageType _t2, QJsonObject _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MessageRouter::requestReceived(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
