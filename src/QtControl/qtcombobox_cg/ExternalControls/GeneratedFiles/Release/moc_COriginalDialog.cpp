/****************************************************************************
** Meta object code from reading C++ file 'COriginalDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../COriginalDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'COriginalDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_COriginalDialog_t {
    QByteArrayData data[6];
    char stringdata0[58];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_COriginalDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_COriginalDialog_t qt_meta_stringdata_COriginalDialog = {
    {
QT_MOC_LITERAL(0, 0, 15), // "COriginalDialog"
QT_MOC_LITERAL(1, 16, 15), // "ncActiveChanged"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 8), // "ncActive"
QT_MOC_LITERAL(4, 42, 13), // "activeChanged"
QT_MOC_LITERAL(5, 56, 1) // "s"

    },
    "COriginalDialog\0ncActiveChanged\0\0"
    "ncActive\0activeChanged\0s"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_COriginalDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,
       4,    1,   27,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::Bool,    5,

       0        // eod
};

void COriginalDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        COriginalDialog *_t = static_cast<COriginalDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->ncActiveChanged((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 1: _t->activeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (COriginalDialog::*_t)(const bool & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&COriginalDialog::ncActiveChanged)) {
                *result = 0;
            }
        }
        {
            typedef void (COriginalDialog::*_t)(bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&COriginalDialog::activeChanged)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject COriginalDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_COriginalDialog.data,
      qt_meta_data_COriginalDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *COriginalDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *COriginalDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_COriginalDialog.stringdata0))
        return static_cast<void*>(const_cast< COriginalDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int COriginalDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void COriginalDialog::ncActiveChanged(const bool & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void COriginalDialog::activeChanged(bool _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
