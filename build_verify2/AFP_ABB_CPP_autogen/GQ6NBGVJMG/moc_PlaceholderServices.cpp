/****************************************************************************
** Meta object code from reading C++ file 'PlaceholderServices.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/services/PlaceholderServices.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PlaceholderServices.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN18PlaceholderServiceE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN18PlaceholderServiceE = QtMocHelpers::stringData(
    "PlaceholderService",
    "messageRequested",
    "",
    "title",
    "message"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN18PlaceholderServiceE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,

       0        // eod
};

Q_CONSTINIT const QMetaObject PlaceholderService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN18PlaceholderServiceE.offsetsAndSizes,
    qt_meta_data_ZN18PlaceholderServiceE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN18PlaceholderServiceE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlaceholderService, std::true_type>,
        // method 'messageRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void PlaceholderService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PlaceholderService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->messageRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PlaceholderService::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &PlaceholderService::messageRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *PlaceholderService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlaceholderService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN18PlaceholderServiceE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PlaceholderService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void PlaceholderService::messageRequested(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
namespace {
struct qt_meta_tag_ZN13SerialServiceE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13SerialServiceE = QtMocHelpers::stringData(
    "SerialService",
    "portCheck",
    "",
    "portOpen",
    "portClose"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13SerialServiceE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x0a,    1 /* Public */,
       3,    0,   33,    2, 0x0a,    2 /* Public */,
       4,    0,   34,    2, 0x0a,    3 /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject SerialService::staticMetaObject = { {
    QMetaObject::SuperData::link<PlaceholderService::staticMetaObject>(),
    qt_meta_stringdata_ZN13SerialServiceE.offsetsAndSizes,
    qt_meta_data_ZN13SerialServiceE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13SerialServiceE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SerialService, std::true_type>,
        // method 'portCheck'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'portOpen'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'portClose'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SerialService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SerialService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->portCheck(); break;
        case 1: _t->portOpen(); break;
        case 2: _t->portClose(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *SerialService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SerialService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13SerialServiceE.stringdata0))
        return static_cast<void*>(this);
    return PlaceholderService::qt_metacast(_clname);
}

int SerialService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PlaceholderService::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}
namespace {
struct qt_meta_tag_ZN12RobotServiceE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN12RobotServiceE = QtMocHelpers::stringData(
    "RobotService",
    "connectRobot",
    "",
    "moveRobot",
    "resetRobot",
    "moveRobotAxes",
    "moveExternalAxis",
    "resetExternalAxis",
    "startPrint",
    "pausePrint",
    "continuePrint",
    "stopPrint"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN12RobotServiceE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x0a,    1 /* Public */,
       3,    0,   75,    2, 0x0a,    2 /* Public */,
       4,    0,   76,    2, 0x0a,    3 /* Public */,
       5,    0,   77,    2, 0x0a,    4 /* Public */,
       6,    0,   78,    2, 0x0a,    5 /* Public */,
       7,    0,   79,    2, 0x0a,    6 /* Public */,
       8,    0,   80,    2, 0x0a,    7 /* Public */,
       9,    0,   81,    2, 0x0a,    8 /* Public */,
      10,    0,   82,    2, 0x0a,    9 /* Public */,
      11,    0,   83,    2, 0x0a,   10 /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject RobotService::staticMetaObject = { {
    QMetaObject::SuperData::link<PlaceholderService::staticMetaObject>(),
    qt_meta_stringdata_ZN12RobotServiceE.offsetsAndSizes,
    qt_meta_data_ZN12RobotServiceE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN12RobotServiceE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<RobotService, std::true_type>,
        // method 'connectRobot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'moveRobot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetRobot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'moveRobotAxes'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'moveExternalAxis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetExternalAxis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startPrint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pausePrint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'continuePrint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopPrint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void RobotService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RobotService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->connectRobot(); break;
        case 1: _t->moveRobot(); break;
        case 2: _t->resetRobot(); break;
        case 3: _t->moveRobotAxes(); break;
        case 4: _t->moveExternalAxis(); break;
        case 5: _t->resetExternalAxis(); break;
        case 6: _t->startPrint(); break;
        case 7: _t->pausePrint(); break;
        case 8: _t->continuePrint(); break;
        case 9: _t->stopPrint(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *RobotService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RobotService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN12RobotServiceE.stringdata0))
        return static_cast<void*>(this);
    return PlaceholderService::qt_metacast(_clname);
}

int RobotService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PlaceholderService::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
