/****************************************************************************
** Meta object code from reading C++ file 'AccountDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../AccountDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AccountDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_AccountDialog_t {
    QByteArrayData data[8];
    char stringdata0[66];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AccountDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AccountDialog_t qt_meta_stringdata_AccountDialog = {
    {
QT_MOC_LITERAL(0, 0, 13), // "AccountDialog"
QT_MOC_LITERAL(1, 14, 12), // "errorVisible"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 7), // "visible"
QT_MOC_LITERAL(4, 36, 13), // "accountAccept"
QT_MOC_LITERAL(5, 50, 3), // "tar"
QT_MOC_LITERAL(6, 54, 7), // "Qt::Key"
QT_MOC_LITERAL(7, 62, 3) // "key"

    },
    "AccountDialog\0errorVisible\0\0visible\0"
    "accountAccept\0tar\0Qt::Key\0key"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AccountDialog[] = {

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
       1,    1,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    2,   27,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::QObjectStar, 0x80000000 | 6,    5,    7,

       0        // eod
};

void AccountDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        AccountDialog *_t = static_cast<AccountDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->errorVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->accountAccept((*reinterpret_cast< QObject*(*)>(_a[1])),(*reinterpret_cast< Qt::Key(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (AccountDialog::*_t)(bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&AccountDialog::errorVisible)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject AccountDialog::staticMetaObject = {
    { &BoxDialogBase::staticMetaObject, qt_meta_stringdata_AccountDialog.data,
      qt_meta_data_AccountDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *AccountDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AccountDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_AccountDialog.stringdata0))
        return static_cast<void*>(const_cast< AccountDialog*>(this));
    return BoxDialogBase::qt_metacast(_clname);
}

int AccountDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BoxDialogBase::qt_metacall(_c, _id, _a);
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
void AccountDialog::errorVisible(bool _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
