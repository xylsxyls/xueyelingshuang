/****************************************************************************
** Meta object code from reading C++ file 'qtcombobox.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../qtcombobox.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qtcombobox.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_qtcombobox_t {
    QByteArrayData data[16];
    char stringdata0[164];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_qtcombobox_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_qtcombobox_t qt_meta_stringdata_qtcombobox = {
    {
QT_MOC_LITERAL(0, 0, 10), // "qtcombobox"
QT_MOC_LITERAL(1, 11, 19), // "currentTextChanged6"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 3), // "str"
QT_MOC_LITERAL(4, 36, 21), // "idComboBoxItemChanged"
QT_MOC_LITERAL(5, 58, 2), // "id"
QT_MOC_LITERAL(6, 61, 15), // "htmlItemPressed"
QT_MOC_LITERAL(7, 77, 5), // "index"
QT_MOC_LITERAL(8, 83, 8), // "modalPop"
QT_MOC_LITERAL(9, 92, 14), // "modalFriendPop"
QT_MOC_LITERAL(10, 107, 10), // "testDialog"
QT_MOC_LITERAL(11, 118, 12), // "onDialogDone"
QT_MOC_LITERAL(12, 131, 7), // "int32_t"
QT_MOC_LITERAL(13, 139, 8), // "dialogId"
QT_MOC_LITERAL(14, 148, 6), // "result"
QT_MOC_LITERAL(15, 155, 8) // "userType"

    },
    "qtcombobox\0currentTextChanged6\0\0str\0"
    "idComboBoxItemChanged\0id\0htmlItemPressed\0"
    "index\0modalPop\0modalFriendPop\0testDialog\0"
    "onDialogDone\0int32_t\0dialogId\0result\0"
    "userType"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_qtcombobox[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x08 /* Private */,
       4,    2,   52,    2, 0x08 /* Private */,
       6,    1,   57,    2, 0x08 /* Private */,
       8,    0,   60,    2, 0x08 /* Private */,
       9,    0,   61,    2, 0x08 /* Private */,
      10,    0,   62,    2, 0x08 /* Private */,
      11,    3,   63,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,    5,    3,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 12, 0x80000000 | 12, 0x80000000 | 12,   13,   14,   15,

       0        // eod
};

void qtcombobox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        qtcombobox *_t = static_cast<qtcombobox *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->currentTextChanged6((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->idComboBoxItemChanged((*reinterpret_cast< qint64(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->htmlItemPressed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->modalPop(); break;
        case 4: _t->modalFriendPop(); break;
        case 5: _t->testDialog(); break;
        case 6: _t->onDialogDone((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< int32_t(*)>(_a[2])),(*reinterpret_cast< int32_t(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObject qtcombobox::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_qtcombobox.data,
      qt_meta_data_qtcombobox,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *qtcombobox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *qtcombobox::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_qtcombobox.stringdata0))
        return static_cast<void*>(const_cast< qtcombobox*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int qtcombobox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE