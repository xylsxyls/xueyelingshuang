/****************************************************************************
** Meta object code from reading C++ file 'DialogShow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../DialogShow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DialogShow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DialogShow_t {
    QByteArrayData data[10];
    char stringdata0[90];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DialogShow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DialogShow_t qt_meta_stringdata_DialogShow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "DialogShow"
QT_MOC_LITERAL(1, 11, 10), // "dialogDone"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 7), // "int32_t"
QT_MOC_LITERAL(4, 31, 8), // "dialogId"
QT_MOC_LITERAL(5, 40, 6), // "result"
QT_MOC_LITERAL(6, 47, 8), // "userType"
QT_MOC_LITERAL(7, 56, 14), // "keyboardAccept"
QT_MOC_LITERAL(8, 71, 10), // "timeUpdate"
QT_MOC_LITERAL(9, 82, 7) // "timeOut"

    },
    "DialogShow\0dialogDone\0\0int32_t\0dialogId\0"
    "result\0userType\0keyboardAccept\0"
    "timeUpdate\0timeOut"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DialogShow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   29,    2, 0x06 /* Public */,
       7,    0,   36,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    1,   37,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 3,    4,    5,    6,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    9,

       0        // eod
};

void DialogShow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        DialogShow *_t = static_cast<DialogShow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->dialogDone((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< int32_t(*)>(_a[2])),(*reinterpret_cast< int32_t(*)>(_a[3]))); break;
        case 1: _t->keyboardAccept(); break;
        case 2: _t->timeUpdate((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (DialogShow::*_t)(int32_t , int32_t , int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogShow::dialogDone)) {
                *result = 0;
            }
        }
        {
            typedef void (DialogShow::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogShow::keyboardAccept)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject DialogShow::staticMetaObject = {
    { &DialogBase::staticMetaObject, qt_meta_stringdata_DialogShow.data,
      qt_meta_data_DialogShow,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *DialogShow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DialogShow::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_DialogShow.stringdata0))
        return static_cast<void*>(const_cast< DialogShow*>(this));
    return DialogBase::qt_metacast(_clname);
}

int DialogShow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DialogBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void DialogShow::dialogDone(int32_t _t1, int32_t _t2, int32_t _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DialogShow::keyboardAccept()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
