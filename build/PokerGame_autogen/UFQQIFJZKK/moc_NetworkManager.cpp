/****************************************************************************
** Meta object code from reading C++ file 'NetworkManager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/network/NetworkManager.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NetworkManager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14NetworkManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto NetworkManager::qt_create_metaobjectdata<qt_meta_tag_ZN14NetworkManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NetworkManager",
        "playerConnected",
        "",
        "playerId",
        "playerDisconnected",
        "dataReceivedFromPlayer",
        "QJsonObject",
        "msg",
        "connected",
        "disconnected",
        "dataReceivedFromServer",
        "errorOccurred",
        "error",
        "onNewConnection",
        "onClientReadyRead",
        "onClientDisconnected",
        "onSocketReadyRead"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'playerConnected'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'playerDisconnected'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'dataReceivedFromPlayer'
        QtMocHelpers::SignalData<void(int, const QJsonObject &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 6, 7 },
        }}),
        // Signal 'connected'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'disconnected'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dataReceivedFromServer'
        QtMocHelpers::SignalData<void(const QJsonObject &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Slot 'onNewConnection'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClientReadyRead'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClientDisconnected'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSocketReadyRead'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NetworkManager, qt_meta_tag_ZN14NetworkManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NetworkManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14NetworkManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14NetworkManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14NetworkManagerE_t>.metaTypes,
    nullptr
} };

void NetworkManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NetworkManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->playerConnected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->playerDisconnected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->dataReceivedFromPlayer((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 3: _t->connected(); break;
        case 4: _t->disconnected(); break;
        case 5: _t->dataReceivedFromServer((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 6: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->onNewConnection(); break;
        case 8: _t->onClientReadyRead(); break;
        case 9: _t->onClientDisconnected(); break;
        case 10: _t->onSocketReadyRead(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(int )>(_a, &NetworkManager::playerConnected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(int )>(_a, &NetworkManager::playerDisconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(int , const QJsonObject & )>(_a, &NetworkManager::dataReceivedFromPlayer, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)()>(_a, &NetworkManager::connected, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)()>(_a, &NetworkManager::disconnected, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QJsonObject & )>(_a, &NetworkManager::dataReceivedFromServer, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & )>(_a, &NetworkManager::errorOccurred, 6))
            return;
    }
}

const QMetaObject *NetworkManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14NetworkManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void NetworkManager::playerConnected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void NetworkManager::playerDisconnected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void NetworkManager::dataReceivedFromPlayer(int _t1, const QJsonObject & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void NetworkManager::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void NetworkManager::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void NetworkManager::dataReceivedFromServer(const QJsonObject & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void NetworkManager::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}
QT_WARNING_POP
