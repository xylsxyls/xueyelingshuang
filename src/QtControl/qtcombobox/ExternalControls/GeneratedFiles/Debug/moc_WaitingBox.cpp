/****************************************************************************
** Meta object code from reading C++ file 'WaitingBox.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../WaitingBox.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WaitingBox.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_WaitingBox_t {
    QByteArrayData data[11];
    char stringdata0[124];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WaitingBox_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WaitingBox_t qt_meta_stringdata_WaitingBox = {
    {
QT_MOC_LITERAL(0, 0, 10), // "WaitingBox"
QT_MOC_LITERAL(1, 11, 10), // "exitSignal"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 14), // "waitingBoxDone"
QT_MOC_LITERAL(4, 38, 11), // "WaitingBox*"
QT_MOC_LITERAL(5, 50, 3), // "box"
QT_MOC_LITERAL(6, 54, 20), // "WaitingBox::ExitEnum"
QT_MOC_LITERAL(7, 75, 8), // "exitEnum"
QT_MOC_LITERAL(8, 84, 17), // "onNcActiveChanged"
QT_MOC_LITERAL(9, 102, 8), // "ncActive"
QT_MOC_LITERAL(10, 111, 12) // "onExitSignal"

    },
    "WaitingBox\0exitSignal\0\0waitingBoxDone\0"
    "WaitingBox*\0box\0WaitingBox::ExitEnum\0"
    "exitEnum\0onNcActiveChanged\0ncActive\0"
    "onExitSignal"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WaitingBox[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,
       3,    2,   35,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    1,   40,    2, 0x08 /* Private */,
      10,    0,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4, 0x80000000 | 6,    5,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,

       0        // eod
};

void WaitingBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        WaitingBox *_t = static_cast<WaitingBox *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->exitSignal(); break;
        case 1: _t->waitingBoxDone((*reinterpret_cast< WaitingBox*(*)>(_a[1])),(*reinterpret_cast< WaitingBox::ExitEnum(*)>(_a[2]))); break;
        case 2: _t->onNcActiveChanged((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 3: _t->onExitSignal(); break;
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
            typedef void (WaitingBox::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&WaitingBox::exitSignal)) {
                *result = 0;
            }
        }
        {
            typedef void (WaitingBox::*_t)(WaitingBox * , WaitingBox::ExitEnum );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&WaitingBox::waitingBoxDone)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject WaitingBox::staticMetaObject = {
    { &COriginalDialog::staticMetaObject, qt_meta_stringdata_WaitingBox.data,
      qt_meta_data_WaitingBox,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *WaitingBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WaitingBox::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_WaitingBox.stringdata0))
        return static_cast<void*>(const_cast< WaitingBox*>(this));
    return COriginalDialog::qt_metacast(_clname);
}

int WaitingBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = COriginalDialog::qt_metacall(_c, _id, _a);
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
void WaitingBox::exitSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void WaitingBox::waitingBoxDone(WaitingBox * _t1, WaitingBox::ExitEnum _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
