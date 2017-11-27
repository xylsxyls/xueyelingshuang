/****************************************************************************
** Meta object code from reading C++ file 'GameInfoWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../GameInfoWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GameInfoWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_GameInfoWidget_t {
    QByteArrayData data[17];
    char stringdata0[288];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GameInfoWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GameInfoWidget_t qt_meta_stringdata_GameInfoWidget = {
    {
QT_MOC_LITERAL(0, 0, 14), // "GameInfoWidget"
QT_MOC_LITERAL(1, 15, 13), // "onSaveClicked"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 21), // "onInviteFriendClicked"
QT_MOC_LITERAL(4, 52, 18), // "onStartGameClicked"
QT_MOC_LITERAL(5, 71, 13), // "onExitClicked"
QT_MOC_LITERAL(6, 85, 17), // "onGameNameChanged"
QT_MOC_LITERAL(7, 103, 21), // "onGamePasswordChanged"
QT_MOC_LITERAL(8, 125, 17), // "onGameModeChanged"
QT_MOC_LITERAL(9, 143, 15), // "onGameLvChanged"
QT_MOC_LITERAL(10, 159, 16), // "onGameMVPChanged"
QT_MOC_LITERAL(11, 176, 16), // "onGameNetChanged"
QT_MOC_LITERAL(12, 193, 18), // "onGameLeaveChanged"
QT_MOC_LITERAL(13, 212, 14), // "onJudgeChanged"
QT_MOC_LITERAL(14, 227, 20), // "onGameSettingClicked"
QT_MOC_LITERAL(15, 248, 23), // "onPersonalRecordClicked"
QT_MOC_LITERAL(16, 272, 15) // "onMyToolClicked"

    },
    "GameInfoWidget\0onSaveClicked\0\0"
    "onInviteFriendClicked\0onStartGameClicked\0"
    "onExitClicked\0onGameNameChanged\0"
    "onGamePasswordChanged\0onGameModeChanged\0"
    "onGameLvChanged\0onGameMVPChanged\0"
    "onGameNetChanged\0onGameLeaveChanged\0"
    "onJudgeChanged\0onGameSettingClicked\0"
    "onPersonalRecordClicked\0onMyToolClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GameInfoWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x06 /* Public */,
       3,    0,   90,    2, 0x06 /* Public */,
       4,    0,   91,    2, 0x06 /* Public */,
       5,    0,   92,    2, 0x06 /* Public */,
       6,    1,   93,    2, 0x06 /* Public */,
       7,    1,   96,    2, 0x06 /* Public */,
       8,    1,   99,    2, 0x06 /* Public */,
       9,    1,  102,    2, 0x06 /* Public */,
      10,    1,  105,    2, 0x06 /* Public */,
      11,    1,  108,    2, 0x06 /* Public */,
      12,    1,  111,    2, 0x06 /* Public */,
      13,    1,  114,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    0,  117,    2, 0x08 /* Private */,
      15,    0,  118,    2, 0x08 /* Private */,
      16,    0,  119,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::Int,    2,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void GameInfoWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        GameInfoWidget *_t = static_cast<GameInfoWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onSaveClicked(); break;
        case 1: _t->onInviteFriendClicked(); break;
        case 2: _t->onStartGameClicked(); break;
        case 3: _t->onExitClicked(); break;
        case 4: _t->onGameNameChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->onGamePasswordChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->onGameModeChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->onGameLvChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->onGameMVPChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->onGameNetChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->onGameLeaveChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->onJudgeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->onGameSettingClicked(); break;
        case 13: _t->onPersonalRecordClicked(); break;
        case 14: _t->onMyToolClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (GameInfoWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onSaveClicked)) {
                *result = 0;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onInviteFriendClicked)) {
                *result = 1;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onStartGameClicked)) {
                *result = 2;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onExitClicked)) {
                *result = 3;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGameNameChanged)) {
                *result = 4;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGamePasswordChanged)) {
                *result = 5;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGameModeChanged)) {
                *result = 6;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGameLvChanged)) {
                *result = 7;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGameMVPChanged)) {
                *result = 8;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGameNetChanged)) {
                *result = 9;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onGameLeaveChanged)) {
                *result = 10;
            }
        }
        {
            typedef void (GameInfoWidget::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&GameInfoWidget::onJudgeChanged)) {
                *result = 11;
            }
        }
    }
}

const QMetaObject GameInfoWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_GameInfoWidget.data,
      qt_meta_data_GameInfoWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *GameInfoWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GameInfoWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_GameInfoWidget.stringdata0))
        return static_cast<void*>(const_cast< GameInfoWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int GameInfoWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void GameInfoWidget::onSaveClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void GameInfoWidget::onInviteFriendClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void GameInfoWidget::onStartGameClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}

// SIGNAL 3
void GameInfoWidget::onExitClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}

// SIGNAL 4
void GameInfoWidget::onGameNameChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void GameInfoWidget::onGamePasswordChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void GameInfoWidget::onGameModeChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void GameInfoWidget::onGameLvChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void GameInfoWidget::onGameMVPChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void GameInfoWidget::onGameNetChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void GameInfoWidget::onGameLeaveChanged(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void GameInfoWidget::onJudgeChanged(int _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_END_MOC_NAMESPACE
