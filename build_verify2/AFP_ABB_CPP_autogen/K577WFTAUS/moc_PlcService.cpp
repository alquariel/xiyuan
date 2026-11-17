/****************************************************************************
** Meta object code from reading C++ file 'PlcService.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/services/plc/PlcService.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PlcService.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10PlcServiceE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10PlcServiceE = QtMocHelpers::stringData(
    "PlcService",
    "messageRequested",
    "",
    "title",
    "message",
    "disconnectPlc",
    "cylinder1",
    "cylinder2",
    "cylinder3",
    "motorEnable",
    "motorReset",
    "motorActuation",
    "parameterInput",
    "setMotorJogMode",
    "setMotorContinuousMode",
    "laserConnect",
    "setTargetTemperature",
    "setLaserPower",
    "toggleTemperatureControl",
    "togglePowerControl",
    "openRedLight",
    "closeRedLight",
    "laserStart",
    "laserStop",
    "setHysteresisRatio"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10PlcServiceE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  140,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,  145,    2, 0x0a,    4 /* Public */,
       6,    0,  146,    2, 0x0a,    5 /* Public */,
       7,    0,  147,    2, 0x0a,    6 /* Public */,
       8,    0,  148,    2, 0x0a,    7 /* Public */,
       9,    0,  149,    2, 0x0a,    8 /* Public */,
      10,    0,  150,    2, 0x0a,    9 /* Public */,
      11,    0,  151,    2, 0x0a,   10 /* Public */,
      12,    0,  152,    2, 0x0a,   11 /* Public */,
      13,    0,  153,    2, 0x0a,   12 /* Public */,
      14,    0,  154,    2, 0x0a,   13 /* Public */,
      15,    0,  155,    2, 0x0a,   14 /* Public */,
      16,    0,  156,    2, 0x0a,   15 /* Public */,
      17,    0,  157,    2, 0x0a,   16 /* Public */,
      18,    0,  158,    2, 0x0a,   17 /* Public */,
      19,    0,  159,    2, 0x0a,   18 /* Public */,
      20,    0,  160,    2, 0x0a,   19 /* Public */,
      21,    0,  161,    2, 0x0a,   20 /* Public */,
      22,    0,  162,    2, 0x0a,   21 /* Public */,
      23,    0,  163,    2, 0x0a,   22 /* Public */,
      24,    0,  164,    2, 0x0a,   23 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,

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

Q_CONSTINIT const QMetaObject PlcService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN10PlcServiceE.offsetsAndSizes,
    qt_meta_data_ZN10PlcServiceE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10PlcServiceE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlcService, std::true_type>,
        // method 'messageRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'disconnectPlc'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cylinder1'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cylinder2'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cylinder3'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'motorEnable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'motorReset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'motorActuation'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'parameterInput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setMotorJogMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setMotorContinuousMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'laserConnect'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setTargetTemperature'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setLaserPower'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleTemperatureControl'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'togglePowerControl'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openRedLight'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'closeRedLight'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'laserStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'laserStop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setHysteresisRatio'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PlcService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PlcService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->messageRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->disconnectPlc(); break;
        case 2: _t->cylinder1(); break;
        case 3: _t->cylinder2(); break;
        case 4: _t->cylinder3(); break;
        case 5: _t->motorEnable(); break;
        case 6: _t->motorReset(); break;
        case 7: _t->motorActuation(); break;
        case 8: _t->parameterInput(); break;
        case 9: _t->setMotorJogMode(); break;
        case 10: _t->setMotorContinuousMode(); break;
        case 11: _t->laserConnect(); break;
        case 12: _t->setTargetTemperature(); break;
        case 13: _t->setLaserPower(); break;
        case 14: _t->toggleTemperatureControl(); break;
        case 15: _t->togglePowerControl(); break;
        case 16: _t->openRedLight(); break;
        case 17: _t->closeRedLight(); break;
        case 18: _t->laserStart(); break;
        case 19: _t->laserStop(); break;
        case 20: _t->setHysteresisRatio(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PlcService::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &PlcService::messageRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *PlcService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlcService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10PlcServiceE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PlcService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void PlcService::messageRequested(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
