/****************************************************************************
** Meta object code from reading C++ file 'GameController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
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
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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

template <> constexpr inline auto GameController::qt_create_metaobjectdata<qt_meta_tag_ZN14GameControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GameController",
        "wsClientJoined",
        "",
        "clientId",
        "name",
        "wsClientAction",
        "QJsonObject",
        "data",
        "wsClientDisconnected",
        "handleTurnStarted",
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
        "handleRoundCompleted",
        "pot",
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
        "onClientDataReceived",
        "onWSClientJoined",
        "onWSClientAction",
        "onWSClientDisconnected",
        "broadcastWebSocketState",
        "startCountdown",
        "stopCountdown",
        "onCountdownTick"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'wsClientJoined'
        QtMocHelpers::SignalData<void(int, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'wsClientAction'
        QtMocHelpers::SignalData<void(int, const QJsonObject &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 6, 7 },
        }}),
        // Signal 'wsClientDisconnected'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'handleTurnStarted'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'handleTurnEnded'
        QtMocHelpers::SlotData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'handleGameOver'
        QtMocHelpers::SlotData<void(int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Slot 'handlePhaseChanged'
        QtMocHelpers::SlotData<void(GameConstants::GamePhase)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 15, 16 },
        }}),
        // Slot 'handlePlayerActed'
        QtMocHelpers::SlotData<void(int, const QString &, int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 }, { QMetaType::QString, 18 }, { QMetaType::Int, 19 },
        }}),
        // Slot 'handleRoundCompleted'
        QtMocHelpers::SlotData<void(int, int)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Int, 21 },
        }}),
        // Slot 'onPlayerCountChanged'
        QtMocHelpers::SlotData<void(int)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 23 },
        }}),
        // Slot 'processAI'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onStartGame'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onFold'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCall'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onRaise'
        QtMocHelpers::SlotData<void(int)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 19 },
        }}),
        // Slot 'onCompare'
        QtMocHelpers::SlotData<void(int)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 30 },
        }}),
        // Slot 'onSeeCards'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCreateRoom'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onJoinRoom'
        QtMocHelpers::SlotData<void(const QString &)>(33, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 34 },
        }}),
        // Slot 'onNetworkDataReceived'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Slot 'onClientDataReceived'
        QtMocHelpers::SlotData<void(int, const QJsonObject &)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'onWSClientJoined'
        QtMocHelpers::SlotData<void(int, const QString &)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 4 },
        }}),
        // Slot 'onWSClientAction'
        QtMocHelpers::SlotData<void(int, const QJsonObject &)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'onWSClientDisconnected'
        QtMocHelpers::SlotData<void(int)>(39, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'broadcastWebSocketState'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'startCountdown'
        QtMocHelpers::SlotData<void(int)>(41, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'stopCountdown'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCountdownTick'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GameController, qt_meta_tag_ZN14GameControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GameController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14GameControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14GameControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14GameControllerE_t>.metaTypes,
    nullptr
} };

void GameController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GameController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->wsClientJoined((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->wsClientAction((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 2: _t->wsClientDisconnected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->handleTurnStarted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->handleTurnEnded((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->handleGameOver((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->handlePhaseChanged((*reinterpret_cast<std::add_pointer_t<GameConstants::GamePhase>>(_a[1]))); break;
        case 7: _t->handlePlayerActed((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 8: _t->handleRoundCompleted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 9: _t->onPlayerCountChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->processAI(); break;
        case 11: _t->onStartGame(); break;
        case 12: _t->onFold(); break;
        case 13: _t->onCall(); break;
        case 14: _t->onRaise((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->onCompare((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 16: _t->onSeeCards(); break;
        case 17: _t->onCreateRoom(); break;
        case 18: _t->onJoinRoom((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->onNetworkDataReceived((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 20: _t->onClientDataReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 21: _t->onWSClientJoined((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 22: _t->onWSClientAction((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 23: _t->onWSClientDisconnected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 24: _t->broadcastWebSocketState(); break;
        case 25: _t->startCountdown((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 26: _t->stopCountdown(); break;
        case 27: _t->onCountdownTick(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GameController::*)(int , const QString & )>(_a, &GameController::wsClientJoined, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameController::*)(int , const QJsonObject & )>(_a, &GameController::wsClientAction, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameController::*)(int )>(_a, &GameController::wsClientDisconnected, 2))
            return;
    }
}

const QMetaObject *GameController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GameController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14GameControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GameController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 28;
    }
    return _id;
}

// SIGNAL 0
void GameController::wsClientJoined(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void GameController::wsClientAction(int _t1, const QJsonObject & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void GameController::wsClientDisconnected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
