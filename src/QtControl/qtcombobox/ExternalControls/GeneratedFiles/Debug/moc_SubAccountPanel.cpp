/****************************************************************************
** Meta object code from reading C++ file 'SubAccountPanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../SubAccountPanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SubAccountPanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_SubAccountItemView_t {
    QByteArrayData data[8];
    char stringdata0[117];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SubAccountItemView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SubAccountItemView_t qt_meta_stringdata_SubAccountItemView = {
    {
QT_MOC_LITERAL(0, 0, 18), // "SubAccountItemView"
QT_MOC_LITERAL(1, 19, 13), // "switchAccount"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 2), // "id"
QT_MOC_LITERAL(4, 37, 17), // "helpButtonClicked"
QT_MOC_LITERAL(5, 55, 13), // "renameAccount"
QT_MOC_LITERAL(6, 69, 21), // "onSwitchButtonClicked"
QT_MOC_LITERAL(7, 91, 25) // "onChangeNameButtonClicked"

    },
    "SubAccountItemView\0switchAccount\0\0id\0"
    "helpButtonClicked\0renameAccount\0"
    "onSwitchButtonClicked\0onChangeNameButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SubAccountItemView[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,
       4,    0,   42,    2, 0x06 /* Public */,
       5,    1,   43,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   46,    2, 0x08 /* Private */,
       7,    0,   47,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::ULongLong,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::ULongLong,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SubAccountItemView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SubAccountItemView *_t = static_cast<SubAccountItemView *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->switchAccount((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 1: _t->helpButtonClicked(); break;
        case 2: _t->renameAccount((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 3: _t->onSwitchButtonClicked(); break;
        case 4: _t->onChangeNameButtonClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (SubAccountItemView::*_t)(quint64 );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountItemView::switchAccount)) {
                *result = 0;
            }
        }
        {
            typedef void (SubAccountItemView::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountItemView::helpButtonClicked)) {
                *result = 1;
            }
        }
        {
            typedef void (SubAccountItemView::*_t)(quint64 );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountItemView::renameAccount)) {
                *result = 2;
            }
        }
    }
}

const QMetaObject SubAccountItemView::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SubAccountItemView.data,
      qt_meta_data_SubAccountItemView,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *SubAccountItemView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SubAccountItemView::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_SubAccountItemView.stringdata0))
        return static_cast<void*>(const_cast< SubAccountItemView*>(this));
    return QWidget::qt_metacast(_clname);
}

int SubAccountItemView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SubAccountItemView::switchAccount(quint64 _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SubAccountItemView::helpButtonClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void SubAccountItemView::renameAccount(quint64 _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
struct qt_meta_stringdata_SubAccountPanel_t {
    QByteArrayData data[7];
    char stringdata0[89];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SubAccountPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SubAccountPanel_t qt_meta_stringdata_SubAccountPanel = {
    {
QT_MOC_LITERAL(0, 0, 15), // "SubAccountPanel"
QT_MOC_LITERAL(1, 16, 16), // "createSubAccount"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 16), // "siwtchSubAccount"
QT_MOC_LITERAL(4, 51, 2), // "id"
QT_MOC_LITERAL(5, 54, 16), // "renameSubAccount"
QT_MOC_LITERAL(6, 71, 17) // "helpButtonClicked"

    },
    "SubAccountPanel\0createSubAccount\0\0"
    "siwtchSubAccount\0id\0renameSubAccount\0"
    "helpButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SubAccountPanel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,
       3,    1,   35,    2, 0x06 /* Public */,
       5,    1,   38,    2, 0x06 /* Public */,
       6,    0,   41,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::ULongLong,    4,
    QMetaType::Void, QMetaType::ULongLong,    4,
    QMetaType::Void,

       0        // eod
};

void SubAccountPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SubAccountPanel *_t = static_cast<SubAccountPanel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->createSubAccount(); break;
        case 1: _t->siwtchSubAccount((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 2: _t->renameSubAccount((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 3: _t->helpButtonClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (SubAccountPanel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountPanel::createSubAccount)) {
                *result = 0;
            }
        }
        {
            typedef void (SubAccountPanel::*_t)(quint64 );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountPanel::siwtchSubAccount)) {
                *result = 1;
            }
        }
        {
            typedef void (SubAccountPanel::*_t)(quint64 );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountPanel::renameSubAccount)) {
                *result = 2;
            }
        }
        {
            typedef void (SubAccountPanel::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&SubAccountPanel::helpButtonClicked)) {
                *result = 3;
            }
        }
    }
}

const QMetaObject SubAccountPanel::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SubAccountPanel.data,
      qt_meta_data_SubAccountPanel,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *SubAccountPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SubAccountPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_SubAccountPanel.stringdata0))
        return static_cast<void*>(const_cast< SubAccountPanel*>(this));
    return QWidget::qt_metacast(_clname);
}

int SubAccountPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void SubAccountPanel::createSubAccount()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void SubAccountPanel::siwtchSubAccount(quint64 _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SubAccountPanel::renameSubAccount(quint64 _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SubAccountPanel::helpButtonClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
