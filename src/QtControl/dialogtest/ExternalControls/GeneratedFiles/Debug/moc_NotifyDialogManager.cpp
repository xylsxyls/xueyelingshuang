/****************************************************************************
** Meta object code from reading C++ file 'NotifyDialogManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../qtcombobox/ExternalControls/NotifyDialogManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NotifyDialogManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_NotifyDialogManager_t {
    QByteArrayData data[8];
    char stringdata0[78];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NotifyDialogManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NotifyDialogManager_t qt_meta_stringdata_NotifyDialogManager = {
    {
QT_MOC_LITERAL(0, 0, 19), // "NotifyDialogManager"
QT_MOC_LITERAL(1, 20, 10), // "dialogDone"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 7), // "int32_t"
QT_MOC_LITERAL(4, 40, 8), // "dialogId"
QT_MOC_LITERAL(5, 49, 6), // "result"
QT_MOC_LITERAL(6, 56, 8), // "userType"
QT_MOC_LITERAL(7, 65, 12) // "onDialogDone"

    },
    "NotifyDialogManager\0dialogDone\0\0int32_t\0"
    "dialogId\0result\0userType\0onDialogDone"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NotifyDialogManager[] = {

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
       1,    3,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    3,   31,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 3,    4,    5,    6,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 3,    4,    5,    6,

       0        // eod
};

void NotifyDialogManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        NotifyDialogManager *_t = static_cast<NotifyDialogManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->dialogDone((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< int32_t(*)>(_a[2])),(*reinterpret_cast< int32_t(*)>(_a[3]))); break;
        case 1: _t->onDialogDone((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< int32_t(*)>(_a[2])),(*reinterpret_cast< int32_t(*)>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (NotifyDialogManager::*_t)(int32_t , int32_t , int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&NotifyDialogManager::dialogDone)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject NotifyDialogManager::staticMetaObject = {
    { &DialogManager::staticMetaObject, qt_meta_stringdata_NotifyDialogManager.data,
      qt_meta_data_NotifyDialogManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *NotifyDialogManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NotifyDialogManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_NotifyDialogManager.stringdata0))
        return static_cast<void*>(const_cast< NotifyDialogManager*>(this));
    return DialogManager::qt_metacast(_clname);
}

int NotifyDialogManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DialogManager::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void NotifyDialogManager::dialogDone(int32_t _t1, int32_t _t2, int32_t _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
