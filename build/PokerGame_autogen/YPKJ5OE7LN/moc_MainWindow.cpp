/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "seeCardsClicked",
        "",
        "foldClicked",
        "checkClicked",
        "callClicked",
        "raiseClicked",
        "amount",
        "compareClicked",
        "targetPlayerId",
        "startGameClicked",
        "playerCountChanged",
        "count",
        "createRoomClicked",
        "joinRoomClicked",
        "address"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'seeCardsClicked'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'foldClicked'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'checkClicked'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'callClicked'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'raiseClicked'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 },
        }}),
        // Signal 'compareClicked'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 },
        }}),
        // Signal 'startGameClicked'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'playerCountChanged'
        QtMocHelpers::SignalData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 12 },
        }}),
        // Signal 'createRoomClicked'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'joinRoomClicked'
        QtMocHelpers::SignalData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->seeCardsClicked(); break;
        case 1: _t->foldClicked(); break;
        case 2: _t->checkClicked(); break;
        case 3: _t->callClicked(); break;
        case 4: _t->raiseClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->compareClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->startGameClicked(); break;
        case 7: _t->playerCountChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->createRoomClicked(); break;
        case 9: _t->joinRoomClicked((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::seeCardsClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::foldClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::checkClicked, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::callClicked, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)(int )>(_a, &MainWindow::raiseClicked, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)(int )>(_a, &MainWindow::compareClicked, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::startGameClicked, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)(int )>(_a, &MainWindow::playerCountChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::createRoomClicked, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)(const QString & )>(_a, &MainWindow::joinRoomClicked, 9))
            return;
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void MainWindow::seeCardsClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MainWindow::foldClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MainWindow::checkClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MainWindow::callClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MainWindow::raiseClicked(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void MainWindow::compareClicked(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void MainWindow::startGameClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MainWindow::playerCountChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void MainWindow::createRoomClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void MainWindow::joinRoomClicked(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}
QT_WARNING_POP
