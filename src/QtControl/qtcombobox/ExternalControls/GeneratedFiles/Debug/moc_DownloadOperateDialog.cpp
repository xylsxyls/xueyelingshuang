/****************************************************************************
** Meta object code from reading C++ file 'DownloadOperateDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../DownloadOperateDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DownloadOperateDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DownloadOperateDialog_t {
    QByteArrayData data[10];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DownloadOperateDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DownloadOperateDialog_t qt_meta_stringdata_DownloadOperateDialog = {
    {
QT_MOC_LITERAL(0, 0, 21), // "DownloadOperateDialog"
QT_MOC_LITERAL(1, 22, 17), // "changeErrorStatus"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 11), // "rateChanged"
QT_MOC_LITERAL(4, 53, 4), // "rate"
QT_MOC_LITERAL(5, 58, 14), // "persentChanged"
QT_MOC_LITERAL(6, 73, 7), // "persent"
QT_MOC_LITERAL(7, 81, 16), // "downloadComplete"
QT_MOC_LITERAL(8, 98, 19), // "onChangeErrorStatus"
QT_MOC_LITERAL(9, 118, 14) // "downloadAccept"

    },
    "DownloadOperateDialog\0changeErrorStatus\0"
    "\0rateChanged\0rate\0persentChanged\0"
    "persent\0downloadComplete\0onChangeErrorStatus\0"
    "downloadAccept"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DownloadOperateDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x06 /* Public */,
       3,    1,   45,    2, 0x06 /* Public */,
       5,    1,   48,    2, 0x06 /* Public */,
       7,    0,   51,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,   52,    2, 0x0a /* Public */,
       9,    0,   53,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DownloadOperateDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        DownloadOperateDialog *_t = static_cast<DownloadOperateDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->changeErrorStatus(); break;
        case 1: _t->rateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->persentChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->downloadComplete(); break;
        case 4: _t->onChangeErrorStatus(); break;
        case 5: _t->downloadAccept(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (DownloadOperateDialog::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DownloadOperateDialog::changeErrorStatus)) {
                *result = 0;
            }
        }
        {
            typedef void (DownloadOperateDialog::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DownloadOperateDialog::rateChanged)) {
                *result = 1;
            }
        }
        {
            typedef void (DownloadOperateDialog::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DownloadOperateDialog::persentChanged)) {
                *result = 2;
            }
        }
        {
            typedef void (DownloadOperateDialog::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DownloadOperateDialog::downloadComplete)) {
                *result = 3;
            }
        }
    }
}

const QMetaObject DownloadOperateDialog::staticMetaObject = {
    { &DialogShow::staticMetaObject, qt_meta_stringdata_DownloadOperateDialog.data,
      qt_meta_data_DownloadOperateDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *DownloadOperateDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DownloadOperateDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_DownloadOperateDialog.stringdata0))
        return static_cast<void*>(const_cast< DownloadOperateDialog*>(this));
    return DialogShow::qt_metacast(_clname);
}

int DownloadOperateDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DialogShow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void DownloadOperateDialog::changeErrorStatus()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void DownloadOperateDialog::rateChanged(int _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DownloadOperateDialog::persentChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DownloadOperateDialog::downloadComplete()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
