/****************************************************************************
** Meta object code from reading C++ file 'AdvertAskDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../AdvertAskDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AdvertAskDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_AdvertAskDialog_t {
    QByteArrayData data[11];
    char stringdata0[108];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AdvertAskDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AdvertAskDialog_t qt_meta_stringdata_AdvertAskDialog = {
    {
QT_MOC_LITERAL(0, 0, 15), // "AdvertAskDialog"
QT_MOC_LITERAL(1, 16, 13), // "advertClicked"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 16), // "AdvertAskDialog*"
QT_MOC_LITERAL(4, 48, 9), // "dialogPtr"
QT_MOC_LITERAL(5, 58, 3), // "url"
QT_MOC_LITERAL(6, 62, 15), // "advertAskAccept"
QT_MOC_LITERAL(7, 78, 3), // "tar"
QT_MOC_LITERAL(8, 82, 7), // "Qt::Key"
QT_MOC_LITERAL(9, 90, 3), // "key"
QT_MOC_LITERAL(10, 94, 13) // "onLinkClicked"

    },
    "AdvertAskDialog\0advertClicked\0\0"
    "AdvertAskDialog*\0dialogPtr\0url\0"
    "advertAskAccept\0tar\0Qt::Key\0key\0"
    "onLinkClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AdvertAskDialog[] = {

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
       6,    2,   34,    2, 0x0a /* Public */,
      10,    1,   39,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::QString,    4,    5,

 // slots: parameters
    QMetaType::Void, QMetaType::QObjectStar, 0x80000000 | 8,    7,    9,
    QMetaType::Void, QMetaType::QUrl,    5,

       0        // eod
};

void AdvertAskDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        AdvertAskDialog *_t = static_cast<AdvertAskDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->advertClicked((*reinterpret_cast< AdvertAskDialog*(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->advertAskAccept((*reinterpret_cast< QObject*(*)>(_a[1])),(*reinterpret_cast< Qt::Key(*)>(_a[2]))); break;
        case 2: _t->onLinkClicked((*reinterpret_cast< const QUrl(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< AdvertAskDialog* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (AdvertAskDialog::*_t)(AdvertAskDialog * , const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&AdvertAskDialog::advertClicked)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject AdvertAskDialog::staticMetaObject = {
    { &DialogShow::staticMetaObject, qt_meta_stringdata_AdvertAskDialog.data,
      qt_meta_data_AdvertAskDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *AdvertAskDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AdvertAskDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_AdvertAskDialog.stringdata0))
        return static_cast<void*>(const_cast< AdvertAskDialog*>(this));
    return DialogShow::qt_metacast(_clname);
}

int AdvertAskDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DialogShow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void AdvertAskDialog::advertClicked(AdvertAskDialog * _t1, const QString & _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
