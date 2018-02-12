/****************************************************************************
** Meta object code from reading C++ file 'RPGCreateRoomWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../RPGCreateRoomWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RPGCreateRoomWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_RPGCreateRoomWidget_t {
    QByteArrayData data[11];
    char stringdata0[151];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RPGCreateRoomWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RPGCreateRoomWidget_t qt_meta_stringdata_RPGCreateRoomWidget = {
    {
QT_MOC_LITERAL(0, 0, 19), // "RPGCreateRoomWidget"
QT_MOC_LITERAL(1, 20, 11), // "exitClicked"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 17), // "createRoomClicked"
QT_MOC_LITERAL(4, 51, 15), // "gameNameChanged"
QT_MOC_LITERAL(5, 67, 19), // "gamePasswordChanged"
QT_MOC_LITERAL(6, 87, 15), // "gameModeChanged"
QT_MOC_LITERAL(7, 103, 12), // "judgeChanged"
QT_MOC_LITERAL(8, 116, 13), // "onExitClicked"
QT_MOC_LITERAL(9, 130, 14), // "onJudgeChanged"
QT_MOC_LITERAL(10, 145, 5) // "state"

    },
    "RPGCreateRoomWidget\0exitClicked\0\0"
    "createRoomClicked\0gameNameChanged\0"
    "gamePasswordChanged\0gameModeChanged\0"
    "judgeChanged\0onExitClicked\0onJudgeChanged\0"
    "state"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RPGCreateRoomWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    1,   56,    2, 0x06 /* Public */,
       5,    1,   59,    2, 0x06 /* Public */,
       6,    1,   62,    2, 0x06 /* Public */,
       7,    1,   65,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,   68,    2, 0x08 /* Private */,
       9,    1,   69,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::Bool,    2,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,

       0        // eod
};

void RPGCreateRoomWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        RPGCreateRoomWidget *_t = static_cast<RPGCreateRoomWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->exitClicked(); break;
        case 1: _t->createRoomClicked(); break;
        case 2: _t->gameNameChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->gamePasswordChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->gameModeChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->judgeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->onExitClicked(); break;
        case 7: _t->onJudgeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (RPGCreateRoomWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&RPGCreateRoomWidget::exitClicked)) {
                *result = 0;
            }
        }
        {
            typedef void (RPGCreateRoomWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&RPGCreateRoomWidget::createRoomClicked)) {
                *result = 1;
            }
        }
        {
            typedef void (RPGCreateRoomWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&RPGCreateRoomWidget::gameNameChanged)) {
                *result = 2;
            }
        }
        {
            typedef void (RPGCreateRoomWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&RPGCreateRoomWidget::gamePasswordChanged)) {
                *result = 3;
            }
        }
        {
            typedef void (RPGCreateRoomWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&RPGCreateRoomWidget::gameModeChanged)) {
                *result = 4;
            }
        }
        {
            typedef void (RPGCreateRoomWidget::*_t)(bool );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&RPGCreateRoomWidget::judgeChanged)) {
                *result = 5;
            }
        }
    }
}

const QMetaObject RPGCreateRoomWidget::staticMetaObject = {
    { &CW3LModalFrame::staticMetaObject, qt_meta_stringdata_RPGCreateRoomWidget.data,
      qt_meta_data_RPGCreateRoomWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *RPGCreateRoomWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RPGCreateRoomWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_RPGCreateRoomWidget.stringdata0))
        return static_cast<void*>(const_cast< RPGCreateRoomWidget*>(this));
    return CW3LModalFrame::qt_metacast(_clname);
}

int RPGCreateRoomWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void RPGCreateRoomWidget::exitClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void RPGCreateRoomWidget::createRoomClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void RPGCreateRoomWidget::gameNameChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void RPGCreateRoomWidget::gamePasswordChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RPGCreateRoomWidget::gameModeChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void RPGCreateRoomWidget::judgeChanged(bool _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
