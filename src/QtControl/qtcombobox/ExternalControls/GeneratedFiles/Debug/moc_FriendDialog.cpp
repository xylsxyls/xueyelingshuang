/****************************************************************************
** Meta object code from reading C++ file 'FriendDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../FriendDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FriendDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_FriendDialog_t {
    QByteArrayData data[8];
    char stringdata0[81];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FriendDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FriendDialog_t qt_meta_stringdata_FriendDialog = {
    {
QT_MOC_LITERAL(0, 0, 12), // "FriendDialog"
QT_MOC_LITERAL(1, 13, 13), // "inviteClicked"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 7), // "int32_t"
QT_MOC_LITERAL(4, 36, 7), // "groupId"
QT_MOC_LITERAL(5, 44, 6), // "userId"
QT_MOC_LITERAL(6, 51, 13), // "onIndiClicked"
QT_MOC_LITERAL(7, 65, 15) // "onInviteClicked"

    },
    "FriendDialog\0inviteClicked\0\0int32_t\0"
    "groupId\0userId\0onIndiClicked\0"
    "onInviteClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FriendDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   34,    2, 0x08 /* Private */,
       7,    0,   35,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    4,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void FriendDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        FriendDialog *_t = static_cast<FriendDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->inviteClicked((*reinterpret_cast< int32_t(*)>(_a[1])),(*reinterpret_cast< int32_t(*)>(_a[2]))); break;
        case 1: _t->onIndiClicked(); break;
        case 2: _t->onInviteClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (FriendDialog::*_t)(int32_t , int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&FriendDialog::inviteClicked)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject FriendDialog::staticMetaObject = {
    { &CW3LModalFrame::staticMetaObject, qt_meta_stringdata_FriendDialog.data,
      qt_meta_data_FriendDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *FriendDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FriendDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_FriendDialog.stringdata0))
        return static_cast<void*>(const_cast< FriendDialog*>(this));
    return CW3LModalFrame::qt_metacast(_clname);
}

int FriendDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CW3LModalFrame::qt_metacall(_c, _id, _a);
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
void FriendDialog::inviteClicked(int32_t _t1, int32_t _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
