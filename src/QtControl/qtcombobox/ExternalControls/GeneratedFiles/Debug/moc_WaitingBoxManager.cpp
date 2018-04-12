/****************************************************************************
** Meta object code from reading C++ file 'WaitingBoxManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../WaitingBoxManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WaitingBoxManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_WaitingBoxManager_t {
    QByteArrayData data[10];
    char stringdata0[109];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WaitingBoxManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WaitingBoxManager_t qt_meta_stringdata_WaitingBoxManager = {
    {
QT_MOC_LITERAL(0, 0, 17), // "WaitingBoxManager"
QT_MOC_LITERAL(1, 18, 14), // "waitingBoxDone"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 7), // "int32_t"
QT_MOC_LITERAL(4, 42, 3), // "key"
QT_MOC_LITERAL(5, 46, 20), // "WaitingBox::ExitEnum"
QT_MOC_LITERAL(6, 67, 8), // "exitEnum"
QT_MOC_LITERAL(7, 76, 16), // "onWaitingBoxDone"
QT_MOC_LITERAL(8, 93, 11), // "WaitingBox*"
QT_MOC_LITERAL(9, 105, 3) // "box"

    },
    "WaitingBoxManager\0waitingBoxDone\0\0"
    "int32_t\0key\0WaitingBox::ExitEnum\0"
    "exitEnum\0onWaitingBoxDone\0WaitingBox*\0"
    "box"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WaitingBoxManager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    2,   29,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 5,    9,    6,

       0        // eod
};

void WaitingBoxManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        WaitingBoxManager *_t = static_cast<WaitingBoxManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->waitingBoxDone((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< WaitingBox::ExitEnum(*)>(_a[2]))); break;
        case 1: _t->onWaitingBoxDone((*reinterpret_cast< WaitingBox*(*)>(_a[1])),(*reinterpret_cast< WaitingBox::ExitEnum(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< WaitingBox* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (WaitingBoxManager::*_t)(int32_t , WaitingBox::ExitEnum );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&WaitingBoxManager::waitingBoxDone)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject WaitingBoxManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_WaitingBoxManager.data,
      qt_meta_data_WaitingBoxManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *WaitingBoxManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WaitingBoxManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_WaitingBoxManager.stringdata0))
        return static_cast<void*>(const_cast< WaitingBoxManager*>(this));
    return QObject::qt_metacast(_clname);
}

int WaitingBoxManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void WaitingBoxManager::waitingBoxDone(int32_t _t1, WaitingBox::ExitEnum _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
