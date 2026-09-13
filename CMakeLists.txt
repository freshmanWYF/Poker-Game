cmake_minimum_required(VERSION 3.16)

project(PokerGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 自动查找 Qt；自定义安装可通过 -DCMAKE_PREFIX_PATH 指定。
find_library(QRENCODE_LIBRARY qrencode)
find_path(QRENCODE_INCLUDE_DIR qrencode.h PATH_SUFFIXES qrencode)

find_package(Qt6 COMPONENTS Widgets Core Gui Network WebSockets REQUIRED)

set(SOURCES
    src/core/Card.cpp
    src/core/Deck.cpp
    src/core/Hand.cpp
    src/core/Player.cpp
    src/core/GameEngine.cpp
    src/ai/SimpleAI.cpp
    src/ai/CautiousAI.cpp
    src/ai/AggressiveAI.cpp
    src/ai/AdaptiveAI.cpp
    src/ui/MainWindow.cpp
    src/ui/PlayerWidget.cpp
    src/ui/CardWidget.cpp
    src/controller/GameController.cpp
    src/utils/Logger.cpp
    src/utils/GameStore.cpp
    src/network/NetworkManager.cpp
    src/network/HttpServer.cpp
    src/network/WebSocketServer.cpp
)

add_library(PokerGameLib STATIC ${SOURCES})
add_executable(PokerGame src/main.cpp resources.qrc)
target_link_libraries(PokerGame PRIVATE PokerGameLib)

target_include_directories(PokerGameLib PUBLIC
    src/core 
    src/ai 
    src/ui 
    src/controller 
    src/utils
    src/network
)

target_link_libraries(PokerGameLib PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network Qt6::WebSockets)

if(QRENCODE_LIBRARY AND QRENCODE_INCLUDE_DIR)
    target_include_directories(PokerGameLib PRIVATE ${QRENCODE_INCLUDE_DIR})
    target_link_libraries(PokerGameLib PRIVATE ${QRENCODE_LIBRARY})
    target_compile_definitions(PokerGameLib PRIVATE HAS_QRENCODE)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 COMPONENTS Test REQUIRED)
    add_executable(PokerGameTests tests/PokerGameTests.cpp resources.qrc)
    target_link_libraries(PokerGameTests PRIVATE PokerGameLib Qt6::Test)
    add_test(NAME PokerGameTests COMMAND PokerGameTests)
    set_tests_properties(PokerGameTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;POKER_SCREENSHOT_DIR=${CMAKE_CURRENT_BINARY_DIR}/screenshots")
    find_program(NODE_EXECUTABLE node)
    if(NODE_EXECUTABLE)
        add_test(NAME WebClientTests COMMAND ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/tests/web-client.test.js)
    endif()
endif()

if(WIN32)
    set_target_properties(PokerGame PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
