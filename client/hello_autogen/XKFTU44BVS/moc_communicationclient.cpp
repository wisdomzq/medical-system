/****************************************************************************
** Meta object code from reading C++ file 'communicationclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/network/src/client/communicationclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'communicationclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CommunicationClient_t {
    QByteArrayData data[15];
    char stringdata0[169];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CommunicationClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CommunicationClient_t qt_meta_stringdata_CommunicationClient = {
    {
QT_MOC_LITERAL(0, 0, 19), // "CommunicationClient"
QT_MOC_LITERAL(1, 20, 9), // "connected"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 12), // "disconnected"
QT_MOC_LITERAL(4, 44, 12), // "jsonReceived"
QT_MOC_LITERAL(5, 57, 3), // "obj"
QT_MOC_LITERAL(6, 61, 13), // "errorOccurred"
QT_MOC_LITERAL(7, 75, 4), // "code"
QT_MOC_LITERAL(8, 80, 7), // "message"
QT_MOC_LITERAL(9, 88, 8), // "sendJson"
QT_MOC_LITERAL(10, 97, 11), // "onConnected"
QT_MOC_LITERAL(11, 109, 14), // "onDisconnected"
QT_MOC_LITERAL(12, 124, 11), // "onReadyRead"
QT_MOC_LITERAL(13, 136, 13), // "sendHeartbeat"
QT_MOC_LITERAL(14, 150, 18) // "onHeartbeatTimeout"

    },
    "CommunicationClient\0connected\0\0"
    "disconnected\0jsonReceived\0obj\0"
    "errorOccurred\0code\0message\0sendJson\0"
    "onConnected\0onDisconnected\0onReadyRead\0"
    "sendHeartbeat\0onHeartbeatTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CommunicationClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   64,    2, 0x06 /* Public */,
       3,    0,   65,    2, 0x06 /* Public */,
       4,    1,   66,    2, 0x06 /* Public */,
       6,    2,   69,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    1,   74,    2, 0x0a /* Public */,
      10,    0,   77,    2, 0x08 /* Private */,
      11,    0,   78,    2, 0x08 /* Private */,
      12,    0,   79,    2, 0x08 /* Private */,
      13,    0,   80,    2, 0x08 /* Private */,
      14,    0,   81,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    7,    8,

 // slots: parameters
    QMetaType::Void, QMetaType::QJsonObject,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CommunicationClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CommunicationClient *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->jsonReceived((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->sendJson((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 5: _t->onConnected(); break;
        case 6: _t->onDisconnected(); break;
        case 7: _t->onReadyRead(); break;
        case 8: _t->sendHeartbeat(); break;
        case 9: _t->onHeartbeatTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CommunicationClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommunicationClient::connected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CommunicationClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommunicationClient::disconnected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CommunicationClient::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommunicationClient::jsonReceived)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CommunicationClient::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommunicationClient::errorOccurred)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CommunicationClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CommunicationClient.data,
    qt_meta_data_CommunicationClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CommunicationClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CommunicationClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CommunicationClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CommunicationClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void CommunicationClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CommunicationClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CommunicationClient::jsonReceived(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CommunicationClient::errorOccurred(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
