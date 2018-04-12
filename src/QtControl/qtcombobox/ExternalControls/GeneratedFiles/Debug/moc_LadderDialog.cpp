/****************************************************************************
** Meta object code from reading C++ file 'LadderDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../LadderDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LadderDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_LadderDialog_t {
    QByteArrayData data[6];
    char stringdata0[89];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LadderDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LadderDialog_t qt_meta_stringdata_LadderDialog = {
    {
QT_MOC_LITERAL(0, 0, 12), // "LadderDialog"
QT_MOC_LITERAL(1, 13, 14), // "onResizeDialog"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 21), // "onSetGameResultSignal"
QT_MOC_LITERAL(4, 51, 26), // "GameResultType::GameResult"
QT_MOC_LITERAL(5, 78, 10) // "gameResult"

    },
    "LadderDialog\0onResizeDialog\0\0"
    "onSetGameResultSignal\0GameResultType::GameResult\0"
    "gameResult"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LadderDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x08 /* Private */,
       3,    1,   25,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,

       0        // eod
};

void LadderDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        LadderDialog *_t = static_cast<LadderDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onResizeDialog(); break;
        case 1: _t->onSetGameResultSignal((*reinterpret_cast< const GameResultType::GameResult(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject LadderDialog::staticMetaObject = {
    { &BattleDialogBase::staticMetaObject, qt_meta_stringdata_LadderDialog.data,
      qt_meta_data_LadderDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *LadderDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LadderDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_LadderDialog.stringdata0))
        return static_cast<void*>(const_cast< LadderDialog*>(this));
    return BattleDialogBase::qt_metacast(_clname);
}

int LadderDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BattleDialogBase::qt_metacall(_c, _id, _a);
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
QT_END_MOC_NAMESPACE
