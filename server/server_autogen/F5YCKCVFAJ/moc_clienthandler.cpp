/****************************************************************************
** Meta object code from reading C++ file 'clienthandler.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/network/src/server/clienthandler.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'clienthandler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ClientHandler_t {
    QByteArrayData data[14];
    char stringdata0[152];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ClientHandler_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ClientHandler_t qt_meta_stringdata_ClientHandler = {
    {
QT_MOC_LITERAL(0, 0, 13), // "ClientHandler"
QT_MOC_LITERAL(1, 14, 12), // "requestReady"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 14), // "ClientHandler*"
QT_MOC_LITERAL(4, 43, 6), // "sender"
QT_MOC_LITERAL(5, 50, 16), // "Protocol::Header"
QT_MOC_LITERAL(6, 67, 6), // "header"
QT_MOC_LITERAL(7, 74, 7), // "payload"
QT_MOC_LITERAL(8, 82, 11), // "sendMessage"
QT_MOC_LITERAL(9, 94, 21), // "Protocol::MessageType"
QT_MOC_LITERAL(10, 116, 4), // "type"
QT_MOC_LITERAL(11, 121, 3), // "obj"
QT_MOC_LITERAL(12, 125, 11), // "onReadyRead"
QT_MOC_LITERAL(13, 137, 14) // "onDisconnected"

    },
    "ClientHandler\0requestReady\0\0ClientHandler*\0"
    "sender\0Protocol::Header\0header\0payload\0"
    "sendMessage\0Protocol::MessageType\0"
    "type\0obj\0onReadyRead\0onDisconnected"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ClientHandler[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    2,   41,    2, 0x0a /* Public */,
      12,    0,   46,    2, 0x08 /* Private */,
      13,    0,   47,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5, QMetaType::QJsonObject,    4,    6,    7,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 9, QMetaType::QJsonObject,   10,   11,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ClientHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClientHandler *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->requestReady((*reinterpret_cast< ClientHandler*(*)>(_a[1])),(*reinterpret_cast< Protocol::Header(*)>(_a[2])),(*reinterpret_cast< QJsonObject(*)>(_a[3]))); break;
        case 1: _t->sendMessage((*reinterpret_cast< Protocol::MessageType(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 2: _t->onReadyRead(); break;
        case 3: _t->onDisconnected(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ClientHandler* >(); break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Protocol::Header >(); break;
            }
            break;
        case 1:
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
            using _t = void (ClientHandler::*)(ClientHandler * , Protocol::Header , QJsonObject );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClientHandler::requestReady)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ClientHandler::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ClientHandler.data,
    qt_meta_data_ClientHandler,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ClientHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClientHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ClientHandler.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ClientHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void ClientHandler::requestReady(ClientHandler * _t1, Protocol::Header _t2, QJsonObject _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
