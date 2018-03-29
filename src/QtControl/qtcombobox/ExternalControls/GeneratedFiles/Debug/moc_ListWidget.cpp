/****************************************************************************
** Meta object code from reading C++ file 'ListWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ListWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ListWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_ListWidget_t {
    QByteArrayData data[5];
    char stringdata0[46];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ListWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ListWidget_t qt_meta_stringdata_ListWidget = {
    {
QT_MOC_LITERAL(0, 0, 10), // "ListWidget"
QT_MOC_LITERAL(1, 11, 11), // "itemPressed"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(4, 41, 4) // "item"

    },
    "ListWidget\0itemPressed\0\0QListWidgetItem*\0"
    "item"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ListWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

       0        // eod
};

void ListWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ListWidget *_t = static_cast<ListWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->itemPressed((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ListWidget::*_t)(QListWidgetItem * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ListWidget::itemPressed)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject ListWidget::staticMetaObject = {
    { &ControlShow<QListWidget>::staticMetaObject, qt_meta_stringdata_ListWidget.data,
      qt_meta_data_ListWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *ListWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ListWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_ListWidget.stringdata0))
        return static_cast<void*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlFont<QListWidget>"))
        return static_cast< ControlFont<QListWidget>*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlItem<QListWidget>"))
        return static_cast< ControlItem<QListWidget>*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlSelf<QListWidget>"))
        return static_cast< ControlSelf<QListWidget>*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlBorderForNormal<QListWidget>"))
        return static_cast< ControlBorderForNormal<QListWidget>*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlBackgroundForNormal<QListWidget>"))
        return static_cast< ControlBackgroundForNormal<QListWidget>*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlItemBorderForNormalHoverDisabled<QListWidget>"))
        return static_cast< ControlItemBorderForNormalHoverDisabled<QListWidget>*>(const_cast< ListWidget*>(this));
    if (!strcmp(_clname, "ControlItemBackgroundForNormalHoverDisabled<QListWidget>"))
        return static_cast< ControlItemBackgroundForNormalHoverDisabled<QListWidget>*>(const_cast< ListWidget*>(this));
    return ControlShow<QListWidget>::qt_metacast(_clname);
}

int ListWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ControlShow<QListWidget>::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void ListWidget::itemPressed(QListWidgetItem * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
