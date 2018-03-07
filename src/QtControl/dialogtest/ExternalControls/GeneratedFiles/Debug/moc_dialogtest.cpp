/****************************************************************************
** Meta object code from reading C++ file 'dialogtest.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../dialogtest.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dialogtest.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_dialogtest_t {
    QByteArrayData data[15];
    char stringdata0[188];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_dialogtest_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_dialogtest_t qt_meta_stringdata_dialogtest = {
    {
QT_MOC_LITERAL(0, 0, 10), // "dialogtest"
QT_MOC_LITERAL(1, 11, 14), // "onPopTipDialog"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 14), // "onPopAskDialog"
QT_MOC_LITERAL(4, 42, 15), // "onPopWaitDialog"
QT_MOC_LITERAL(5, 58, 16), // "onPopInputDialog"
QT_MOC_LITERAL(6, 75, 15), // "onShowTipDialog"
QT_MOC_LITERAL(7, 91, 17), // "onShowLoginDialog"
QT_MOC_LITERAL(8, 109, 15), // "onShowAskDialog"
QT_MOC_LITERAL(9, 125, 16), // "onShowDestroyAll"
QT_MOC_LITERAL(10, 142, 12), // "onDialogDone"
QT_MOC_LITERAL(11, 155, 7), // "int32_t"
QT_MOC_LITERAL(12, 163, 8), // "dialogId"
QT_MOC_LITERAL(13, 172, 6), // "result"
QT_MOC_LITERAL(14, 179, 8) // "userType"

    },
    "dialogtest\0onPopTipDialog\0\0onPopAskDialog\0"
    "onPopWaitDialog\0onPopInputDialog\0"
    "onShowTipDialog\0onShowLoginDialog\0"
    "onShowAskDialog\0onShowDestroyAll\0"
    "onDialogDone\0int32_t\0dialogId\0result\0"
    "userType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_dialogtest[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x08 /* Private */,
       3,    0,   60,    2, 0x08 /* Private */,
       4,    0,   61,    2, 0x08 /* Private */,
       5,    0,   62,    2, 0x08 /* Private */,
       6,    0,   63,    2, 0x08 /* Private */,
       7,    0,   64,    2, 0x08 /* Private */,
       8,    0,   65,    2, 0x08 /* Private */,
       9,    0,   66,    2, 0x08 /* Private */,
      10,    3,   67,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11, 0x80000000 | 11, 0x80000000 | 11,   12,   13,   14,

       0        // eod
};

void dialogtest::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        dialogtest *_t = static_cast<dialogtest *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onPopTipDialog(); break;
        case 1: _t->onPopAskDialog(); break;
        case 2: _t->onPopWaitDialog(); break;
        case 3: _t->onPopInputDialog(); break;
        case 4: _t->onShowTipDialog(); break;
        case 5: _t->onShowLoginDialog(); break;
        case 6: _t->onShowAskDialog(); break;
        case 7: _t->onShowDestroyAll(); break;
        case 8: _t->onDialogDone((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< int32_t(*)>(_a[2])),(*reinterpret_cast< int32_t(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObject dialogtest::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_dialogtest.data,
      qt_meta_data_dialogtest,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *dialogtest::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dialogtest::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_dialogtest.stringdata0))
        return static_cast<void*>(const_cast< dialogtest*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int dialogtest::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
