/****************************************************************************
** Meta object code from reading C++ file 'GameEngine.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/GameEngine.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GameEngine.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10GameEngineE_t {};
} // unnamed namespace

template <> constexpr inline auto GameEngine::qt_create_metaobjectdata<qt_meta_tag_ZN10GameEngineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GameEngine",
        "gameStateChanged",
        "",
        "turnStarted",
        "playerId",
        "playerActed",
        "action",
        "amount",
        "potChanged",
        "newPot",
        "betChanged",
        "newBet",
        "phaseChanged",
        "GameConstants::GamePhase",
        "newPhase",
        "gameOver",
        "winnerId",
        "compareResult",
        "loserId",
        "winnerType",
        "loserType",
        "roundCompleted",
        "pot"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'gameStateChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'turnStarted'
        QtMocHelpers::SignalData<void(int)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Signal 'playerActed'
        QtMocHelpers::SignalData<void(int, const QString &, int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 }, { QMetaType::QString, 6 }, { QMetaType::Int, 7 },
        }}),
        // Signal 'potChanged'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 },
        }}),
        // Signal 'betChanged'
        QtMocHelpers::SignalData<void(int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 },
        }}),
        // Signal 'phaseChanged'
        QtMocHelpers::SignalData<void(GameConstants::GamePhase)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Signal 'gameOver'
        QtMocHelpers::SignalData<void(int)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Signal 'compareResult'
        QtMocHelpers::SignalData<void(int, int, const QString &, const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 16 }, { QMetaType::Int, 18 }, { QMetaType::QString, 19 }, { QMetaType::QString, 20 },
        }}),
        // Signal 'roundCompleted'
        QtMocHelpers::SignalData<void(int, int)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 16 }, { QMetaType::Int, 22 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GameEngine, qt_meta_tag_ZN10GameEngineE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GameEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10GameEngineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10GameEngineE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10GameEngineE_t>.metaTypes,
    nullptr
} };

void GameEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GameEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->gameStateChanged(); break;
        case 1: _t->turnStarted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->playerActed((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 3: _t->potChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->betChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->phaseChanged((*reinterpret_cast<std::add_pointer_t<GameConstants::GamePhase>>(_a[1]))); break;
        case 6: _t->gameOver((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->compareResult((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 8: _t->roundCompleted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)()>(_a, &GameEngine::gameStateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int )>(_a, &GameEngine::turnStarted, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int , const QString & , int )>(_a, &GameEngine::playerActed, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int )>(_a, &GameEngine::potChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int )>(_a, &GameEngine::betChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(GameConstants::GamePhase )>(_a, &GameEngine::phaseChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int )>(_a, &GameEngine::gameOver, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int , int , const QString & , const QString & )>(_a, &GameEngine::compareResult, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (GameEngine::*)(int , int )>(_a, &GameEngine::roundCompleted, 8))
            return;
    }
}

const QMetaObject *GameEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GameEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10GameEngineE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GameEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void GameEngine::gameStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GameEngine::turnStarted(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void GameEngine::playerActed(int _t1, const QString & _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2, _t3);
}

// SIGNAL 3
void GameEngine::potChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void GameEngine::betChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void GameEngine::phaseChanged(GameConstants::GamePhase _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void GameEngine::gameOver(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void GameEngine::compareResult(int _t1, int _t2, const QString & _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 8
void GameEngine::roundCompleted(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}
QT_WARNING_POP
