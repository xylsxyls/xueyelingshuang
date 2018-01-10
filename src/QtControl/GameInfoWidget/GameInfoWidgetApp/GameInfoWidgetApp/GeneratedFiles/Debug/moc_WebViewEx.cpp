/****************************************************************************
** Meta object code from reading C++ file 'WebViewEx.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../qtcombobox/WebViewEx.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WebViewEx.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_WebPage_t {
    QByteArrayData data[1];
    char stringdata0[8];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WebPage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WebPage_t qt_meta_stringdata_WebPage = {
    {
QT_MOC_LITERAL(0, 0, 7) // "WebPage"

    },
    "WebPage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WebPage[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void WebPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject WebPage::staticMetaObject = {
    { &QWebPage::staticMetaObject, qt_meta_stringdata_WebPage.data,
      qt_meta_data_WebPage,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *WebPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WebPage::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_WebPage.stringdata0))
        return static_cast<void*>(const_cast< WebPage*>(this));
    return QWebPage::qt_metacast(_clname);
}

int WebPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWebPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_CWebViewEx_t {
    QByteArrayData data[8];
    char stringdata0[82];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CWebViewEx_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CWebViewEx_t qt_meta_stringdata_CWebViewEx = {
    {
QT_MOC_LITERAL(0, 0, 10), // "CWebViewEx"
QT_MOC_LITERAL(1, 11, 15), // "realAddJSObject"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 11), // "addJSObject"
QT_MOC_LITERAL(4, 40, 11), // "object_name"
QT_MOC_LITERAL(5, 52, 8), // "c_object"
QT_MOC_LITERAL(6, 61, 18), // "evaluateJavaScript"
QT_MOC_LITERAL(7, 80, 1) // "x"

    },
    "CWebViewEx\0realAddJSObject\0\0addJSObject\0"
    "object_name\0c_object\0evaluateJavaScript\0"
    "x"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CWebViewEx[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x09 /* Protected */,

 // methods: name, argc, parameters, tag, flags
       3,    2,   30,    2, 0x02 /* Public */,
       6,    1,   35,    2, 0x02 /* Public */,

 // slots: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QObjectStar,    4,    5,
    QMetaType::QVariant, QMetaType::QString,    7,

       0        // eod
};

void CWebViewEx::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CWebViewEx *_t = static_cast<CWebViewEx *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->realAddJSObject(); break;
        case 1: _t->addJSObject((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QObject*(*)>(_a[2]))); break;
        case 2: { QVariant _r = _t->evaluateJavaScript((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariant*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObject CWebViewEx::staticMetaObject = {
    { &QWebView::staticMetaObject, qt_meta_stringdata_CWebViewEx.data,
      qt_meta_data_CWebViewEx,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *CWebViewEx::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CWebViewEx::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_CWebViewEx.stringdata0))
        return static_cast<void*>(const_cast< CWebViewEx*>(this));
    return QWebView::qt_metacast(_clname);
}

int CWebViewEx::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWebView::qt_metacall(_c, _id, _a);
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
QT_END_MOC_NAMESPACE
