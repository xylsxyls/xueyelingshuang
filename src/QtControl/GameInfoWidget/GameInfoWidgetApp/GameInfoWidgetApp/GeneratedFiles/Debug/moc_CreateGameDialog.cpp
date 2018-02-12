/****************************************************************************
** Meta object code from reading C++ file 'CreateGameDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../CreateGameDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CreateGameDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CreateGameDialog_t {
    QByteArrayData data[10];
    char stringdata0[158];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CreateGameDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CreateGameDialog_t qt_meta_stringdata_CreateGameDialog = {
    {
QT_MOC_LITERAL(0, 0, 16), // "CreateGameDialog"
QT_MOC_LITERAL(1, 17, 11), // "exitClicked"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 17), // "createRoomClicked"
QT_MOC_LITERAL(4, 48, 17), // "mapVersionChanged"
QT_MOC_LITERAL(5, 66, 19), // "gamePasswordChanged"
QT_MOC_LITERAL(6, 86, 15), // "gameModeChanged"
QT_MOC_LITERAL(7, 102, 20), // "challengeModeChanged"
QT_MOC_LITERAL(8, 123, 20), // "challengeCostChanged"
QT_MOC_LITERAL(9, 144, 13) // "onExitClicked"

    },
    "CreateGameDialog\0exitClicked\0\0"
    "createRoomClicked\0mapVersionChanged\0"
    "gamePasswordChanged\0gameModeChanged\0"
    "challengeModeChanged\0challengeCostChanged\0"
    "onExitClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CreateGameDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    1,   56,    2, 0x06 /* Public */,
       5,    1,   59,    2, 0x06 /* Public */,
       6,    1,   62,    2, 0x06 /* Public */,
       7,    1,   65,    2, 0x06 /* Public */,
       8,    1,   68,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    0,   71,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void CreateGameDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CreateGameDialog *_t = static_cast<CreateGameDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->exitClicked(); break;
        case 1: _t->createRoomClicked(); break;
        case 2: _t->mapVersionChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->gamePasswordChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->gameModeChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->challengeModeChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->challengeCostChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->onExitClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (CreateGameDialog::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::exitClicked)) {
                *result = 0;
            }
        }
        {
            typedef void (CreateGameDialog::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::createRoomClicked)) {
                *result = 1;
            }
        }
        {
            typedef void (CreateGameDialog::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::mapVersionChanged)) {
                *result = 2;
            }
        }
        {
            typedef void (CreateGameDialog::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::gamePasswordChanged)) {
                *result = 3;
            }
        }
        {
            typedef void (CreateGameDialog::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::gameModeChanged)) {
                *result = 4;
            }
        }
        {
            typedef void (CreateGameDialog::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::challengeModeChanged)) {
                *result = 5;
            }
        }
        {
            typedef void (CreateGameDialog::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CreateGameDialog::challengeCostChanged)) {
                *result = 6;
            }
        }
    }
}

const QMetaObject CreateGameDialog::staticMetaObject = {
    { &CW3LModalFrame::staticMetaObject, qt_meta_stringdata_CreateGameDialog.data,
      qt_meta_data_CreateGameDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *CreateGameDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CreateGameDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_CreateGameDialog.stringdata0))
        return static_cast<void*>(const_cast< CreateGameDialog*>(this));
    return CW3LModalFrame::qt_metacast(_clname);
}

int CreateGameDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CW3LModalFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void CreateGameDialog::exitClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void CreateGameDialog::createRoomClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void CreateGameDialog::mapVersionChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CreateGameDialog::gamePasswordChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CreateGameDialog::gameModeChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CreateGameDialog::challengeModeChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CreateGameDialog::challengeCostChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_END_MOC_NAMESPACE
