/****************************************************************************
** Meta object code from reading C++ file 'BattleDialogBase.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../BattleDialogBase.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BattleDialogBase.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_BattleDialogBase_t {
    QByteArrayData data[8];
    char stringdata0[121];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BattleDialogBase_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BattleDialogBase_t qt_meta_stringdata_BattleDialogBase = {
    {
QT_MOC_LITERAL(0, 0, 16), // "BattleDialogBase"
QT_MOC_LITERAL(1, 17, 12), // "resizeDialog"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 19), // "setGameResultSignal"
QT_MOC_LITERAL(4, 51, 26), // "GameResultType::GameResult"
QT_MOC_LITERAL(5, 78, 10), // "gameResult"
QT_MOC_LITERAL(6, 89, 15), // "onExpandClicked"
QT_MOC_LITERAL(7, 105, 15) // "onShrinkClicked"

    },
    "BattleDialogBase\0resizeDialog\0\0"
    "setGameResultSignal\0GameResultType::GameResult\0"
    "gameResult\0onExpandClicked\0onShrinkClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BattleDialogBase[] = {

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
       3,    1,   35,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   38,    2, 0x08 /* Private */,
       7,    0,   39,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void BattleDialogBase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        BattleDialogBase *_t = static_cast<BattleDialogBase *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->resizeDialog(); break;
        case 1: _t->setGameResultSignal((*reinterpret_cast< const GameResultType::GameResult(*)>(_a[1]))); break;
        case 2: _t->onExpandClicked(); break;
        case 3: _t->onShrinkClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (BattleDialogBase::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&BattleDialogBase::resizeDialog)) {
                *result = 0;
            }
        }
        {
            typedef void (BattleDialogBase::*_t)(const GameResultType::GameResult & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&BattleDialogBase::setGameResultSignal)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject BattleDialogBase::staticMetaObject = {
    { &CW3LModalFrame::staticMetaObject, qt_meta_stringdata_BattleDialogBase.data,
      qt_meta_data_BattleDialogBase,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *BattleDialogBase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BattleDialogBase::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_BattleDialogBase.stringdata0))
        return static_cast<void*>(const_cast< BattleDialogBase*>(this));
    return CW3LModalFrame::qt_metacast(_clname);
}

int BattleDialogBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CW3LModalFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void BattleDialogBase::resizeDialog()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void BattleDialogBase::setGameResultSignal(const GameResultType::GameResult & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
