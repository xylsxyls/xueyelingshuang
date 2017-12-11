/****************************************************************************
** Meta object code from reading C++ file 'ComboBox.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ComboBox.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ComboBox.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_ComboBox_t {
    QByteArrayData data[8];
    char stringdata0[82];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ComboBox_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ComboBox_t qt_meta_stringdata_ComboBox = {
    {
QT_MOC_LITERAL(0, 0, 8), // "ComboBox"
QT_MOC_LITERAL(1, 9, 11), // "itemPressed"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 5), // "index"
QT_MOC_LITERAL(4, 28, 15), // "listItemEntered"
QT_MOC_LITERAL(5, 44, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(6, 61, 4), // "item"
QT_MOC_LITERAL(7, 66, 15) // "listItemPressed"

    },
    "ComboBox\0itemPressed\0\0index\0listItemEntered\0"
    "QListWidgetItem*\0item\0listItemPressed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ComboBox[] = {

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
       1,    1,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   32,    2, 0x08 /* Private */,
       7,    1,   35,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,

       0        // eod
};

void ComboBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ComboBox *_t = static_cast<ComboBox *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->itemPressed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->listItemEntered((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        case 2: _t->listItemPressed((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ComboBox::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ComboBox::itemPressed)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject ComboBox::staticMetaObject = {
    { &ControlShow<QComboBox>::staticMetaObject, qt_meta_stringdata_ComboBox.data,
      qt_meta_data_ComboBox,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *ComboBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ComboBox::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_ComboBox.stringdata0))
        return static_cast<void*>(const_cast< ComboBox*>(this));
    if (!strcmp(_clname, "ControlFont<QComboBox>"))
        return static_cast< ControlFont<QComboBox>*>(const_cast< ComboBox*>(this));
    if (!strcmp(_clname, "ControlSelf<QComboBox>"))
        return static_cast< ControlSelf<QComboBox>*>(const_cast< ComboBox*>(this));
    if (!strcmp(_clname, "ControlBorderForNormalHoverPressedDisabled<QComboBox>"))
        return static_cast< ControlBorderForNormalHoverPressedDisabled<QComboBox>*>(const_cast< ComboBox*>(this));
    if (!strcmp(_clname, "ControlBackgroundForNormalHoverPressedDisabled<QComboBox>"))
        return static_cast< ControlBackgroundForNormalHoverPressedDisabled<QComboBox>*>(const_cast< ComboBox*>(this));
    return ControlShow<QComboBox>::qt_metacast(_clname);
}

int ComboBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ControlShow<QComboBox>::qt_metacall(_c, _id, _a);
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
void ComboBox::itemPressed(int _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
