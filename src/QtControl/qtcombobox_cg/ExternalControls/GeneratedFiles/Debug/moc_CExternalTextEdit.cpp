/****************************************************************************
** Meta object code from reading C++ file 'CExternalTextEdit.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../CExternalTextEdit.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CExternalTextEdit.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CExternalTextEdit_t {
    QByteArrayData data[12];
    char stringdata0[186];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CExternalTextEdit_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CExternalTextEdit_t qt_meta_stringdata_CExternalTextEdit = {
    {
QT_MOC_LITERAL(0, 0, 17), // "CExternalTextEdit"
QT_MOC_LITERAL(1, 18, 12), // "enterPressed"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 16), // "ctrlEnterPressed"
QT_MOC_LITERAL(4, 49, 11), // "altCPressed"
QT_MOC_LITERAL(5, 61, 11), // "altSPressed"
QT_MOC_LITERAL(6, 73, 30), // "updateGifResourceForExpression"
QT_MOC_LITERAL(7, 104, 18), // "insertFromMimeData"
QT_MOC_LITERAL(8, 123, 16), // "const QMimeData*"
QT_MOC_LITERAL(9, 140, 6), // "source"
QT_MOC_LITERAL(10, 147, 27), // "createMimeDataFromSelection"
QT_MOC_LITERAL(11, 175, 10) // "QMimeData*"

    },
    "CExternalTextEdit\0enterPressed\0\0"
    "ctrlEnterPressed\0altCPressed\0altSPressed\0"
    "updateGifResourceForExpression\0"
    "insertFromMimeData\0const QMimeData*\0"
    "source\0createMimeDataFromSelection\0"
    "QMimeData*"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CExternalTextEdit[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x06 /* Public */,
       3,    0,   50,    2, 0x06 /* Public */,
       4,    0,   51,    2, 0x06 /* Public */,
       5,    0,   52,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   53,    2, 0x08 /* Private */,
       7,    1,   54,    2, 0x08 /* Private */,
      10,    0,   57,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    0x80000000 | 11,

       0        // eod
};

void CExternalTextEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CExternalTextEdit *_t = static_cast<CExternalTextEdit *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->enterPressed(); break;
        case 1: _t->ctrlEnterPressed(); break;
        case 2: _t->altCPressed(); break;
        case 3: _t->altSPressed(); break;
        case 4: _t->updateGifResourceForExpression(); break;
        case 5: _t->insertFromMimeData((*reinterpret_cast< const QMimeData*(*)>(_a[1]))); break;
        case 6: { QMimeData* _r = _t->createMimeDataFromSelection();
            if (_a[0]) *reinterpret_cast< QMimeData**>(_a[0]) = _r; }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (CExternalTextEdit::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CExternalTextEdit::enterPressed)) {
                *result = 0;
            }
        }
        {
            typedef void (CExternalTextEdit::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CExternalTextEdit::ctrlEnterPressed)) {
                *result = 1;
            }
        }
        {
            typedef void (CExternalTextEdit::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CExternalTextEdit::altCPressed)) {
                *result = 2;
            }
        }
        {
            typedef void (CExternalTextEdit::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&CExternalTextEdit::altSPressed)) {
                *result = 3;
            }
        }
    }
}

const QMetaObject CExternalTextEdit::staticMetaObject = {
    { &QTextEdit::staticMetaObject, qt_meta_stringdata_CExternalTextEdit.data,
      qt_meta_data_CExternalTextEdit,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *CExternalTextEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CExternalTextEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_CExternalTextEdit.stringdata0))
        return static_cast<void*>(const_cast< CExternalTextEdit*>(this));
    return QTextEdit::qt_metacast(_clname);
}

int CExternalTextEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void CExternalTextEdit::enterPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void CExternalTextEdit::ctrlEnterPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void CExternalTextEdit::altCPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}

// SIGNAL 3
void CExternalTextEdit::altSPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
