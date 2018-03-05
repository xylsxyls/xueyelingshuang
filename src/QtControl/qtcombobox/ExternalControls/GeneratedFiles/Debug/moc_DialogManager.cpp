/****************************************************************************
** Meta object code from reading C++ file 'DialogManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../DialogManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DialogManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DialogManager_t {
    QByteArrayData data[10];
    char stringdata0[117];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DialogManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DialogManager_t qt_meta_stringdata_DialogManager = {
    {
QT_MOC_LITERAL(0, 0, 13), // "DialogManager"
QT_MOC_LITERAL(1, 14, 12), // "changeToBack"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 7), // "int32_t"
QT_MOC_LITERAL(4, 36, 8), // "dialogId"
QT_MOC_LITERAL(5, 45, 13), // "downloadAgain"
QT_MOC_LITERAL(6, 59, 14), // "cancelDownload"
QT_MOC_LITERAL(7, 74, 16), // "useOtherDownload"
QT_MOC_LITERAL(8, 91, 16), // "copyDownloadAddr"
QT_MOC_LITERAL(9, 108, 8) // "copyPath"

    },
    "DialogManager\0changeToBack\0\0int32_t\0"
    "dialogId\0downloadAgain\0cancelDownload\0"
    "useOtherDownload\0copyDownloadAddr\0"
    "copyPath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DialogManager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       5,    1,   47,    2, 0x06 /* Public */,
       6,    1,   50,    2, 0x06 /* Public */,
       7,    1,   53,    2, 0x06 /* Public */,
       8,    1,   56,    2, 0x06 /* Public */,
       9,    1,   59,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,

       0        // eod
};

void DialogManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        DialogManager *_t = static_cast<DialogManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->changeToBack((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        case 1: _t->downloadAgain((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        case 2: _t->cancelDownload((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        case 3: _t->useOtherDownload((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        case 4: _t->copyDownloadAddr((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        case 5: _t->copyPath((*reinterpret_cast< int32_t(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (DialogManager::*_t)(int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogManager::changeToBack)) {
                *result = 0;
            }
        }
        {
            typedef void (DialogManager::*_t)(int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogManager::downloadAgain)) {
                *result = 1;
            }
        }
        {
            typedef void (DialogManager::*_t)(int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogManager::cancelDownload)) {
                *result = 2;
            }
        }
        {
            typedef void (DialogManager::*_t)(int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogManager::useOtherDownload)) {
                *result = 3;
            }
        }
        {
            typedef void (DialogManager::*_t)(int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogManager::copyDownloadAddr)) {
                *result = 4;
            }
        }
        {
            typedef void (DialogManager::*_t)(int32_t );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DialogManager::copyPath)) {
                *result = 5;
            }
        }
    }
}

const QMetaObject DialogManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DialogManager.data,
      qt_meta_data_DialogManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *DialogManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DialogManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_DialogManager.stringdata0))
        return static_cast<void*>(const_cast< DialogManager*>(this));
    return QObject::qt_metacast(_clname);
}

int DialogManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void DialogManager::changeToBack(int32_t _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DialogManager::downloadAgain(int32_t _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DialogManager::cancelDownload(int32_t _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DialogManager::useOtherDownload(int32_t _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DialogManager::copyDownloadAddr(int32_t _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void DialogManager::copyPath(int32_t _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
