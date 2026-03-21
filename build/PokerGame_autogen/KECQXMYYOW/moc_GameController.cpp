/****************************************************************************
** Meta object code from reading C++ file 'GameController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/controller/GameController.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GameController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14GameControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14GameControllerE = QtMocHelpers::stringData(
    "GameController",
    "handleTurnStarted",
    "",
    "playerId",
    "handleTurnEnded",
    "handleGameOver",
    "winnerId",
    "handlePhaseChanged",
    "GameConstants::GamePhase",
    "phase",
    "handlePlayerActed",
    "action",
    "amount",
    "onPlayerCountChanged",
    "count",
    "processAI",
    "onStartGame",
    "onFold",
    "onCall",
    "onRaise",
    "onCompare",
    "targetId",
    "onSeeCards",
    "onCreateRoom",
    "onJoinRoom",
    "address",
    "onNetworkDataReceived",
    "data",
    "onClientDataReceived",
    "clientId"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14GameControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  116,    2, 0x0a,    1 /* Public */,
       4,    1,  119,    2, 0x0a,    3 /* Public */,
       5,    1,  122,    2, 0x0a,    5 /* Public */,
       7,    1,  125,    2, 0x0a,    7 /* Public */,
      10,    3,  128,    2, 0x0a,    9 /* Public */,
      13,    1,  135,    2, 0x0a,   13 /* Public */,
      15,    0,  138,    2, 0x0a,   15 /* Public */,
      16,    0,  139,    2, 0x0a,   16 /* Public */,
      17,    0,  140,    2, 0x0a,   17 /* Public */,
      18,    0,  141,    2, 0x0a,   18 /* Public */,
      19,    1,  142,    2, 0x0a,   19 /* Public */,
      20,    1,  145,    2, 0x0a,   21 /* Public */,
      22,    0,  148,    2, 0x0a,   23 /* Public */,
      23,    0,  149,    2, 0x0a,   24 /* Public */,
      24,    1,  150,    2, 0x0a,   25 /* Public */,
      26,    1,  153,    2, 0x0a,   27 /* Public */,
      28,    2,  156,    2, 0x0a,   29 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::Int,    3,   11,   12,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Int,   21,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QJsonObject,   27,
    QMetaType::Void, QMetaType::Int, QMetaType::QJsonObject,   29,   27,

       0        // eod
};

Q_CONSTINIT const QMetaObject GameController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN14GameControllerE.offsetsAndSizes,
    qt_meta_data_ZN14GameControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14GameControllerE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GameController, std::true_type>,
        // method 'handleTurnStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'handleTurnEnded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'handleGameOver'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'handlePhaseChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<GameConstants::GamePhase, std::false_type>,
        // method 'handlePlayerActed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onPlayerCountChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'processAI'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStartGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFold'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCall'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRaise'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onCompare'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onSeeCards'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCreateRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onJoinRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onNetworkDataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>,
        // method 'onClientDataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>
    >,
    nullptr
} };

void GameController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GameController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->handleTurnStarted((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->handleTurnEnded((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->handleGameOver((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->handlePhaseChanged((*reinterpret_cast< std::add_pointer_t<GameConstants::GamePhase>>(_a[1]))); break;
        case 4: _t->handlePlayerActed((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3]))); break;
        case 5: _t->onPlayerCountChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->processAI(); break;
        case 7: _t->onStartGame(); break;
        case 8: _t->onFold(); break;
        case 9: _t->onCall(); break;
        case 10: _t->onRaise((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->onCompare((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->onSeeCards(); break;
        case 13: _t->onCreateRoom(); break;
        case 14: _t->onJoinRoom((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->onNetworkDataReceived((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 16: _t->onClientDataReceived((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject *GameController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GameController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14GameControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GameController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    return _id;
}
QT_WARNING_POP
