cmake_minimum_required(VERSION 3.16)

project(PokerGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Linux 上自动查找 Qt，Windows 上使用指定路径
if(WIN32)
    set(CMAKE_PREFIX_PATH "D:/QT/6.8.3/mingw_64")
else()
    # Linux: 让 CMake 自动找到系统 Qt
    set(Qt6_DIR)
    # 查找 libqrencode（二维码生成）
    find_library(QRENCODE_LIBRARY qrencode)
    find_path(QRENCODE_INCLUDE_DIR qrencode.h PATH_SUFFIXES qrencode)
endif()

find_package(Qt6 COMPONENTS Widgets Core Gui Network WebSockets REQUIRED)

set(SOURCES
    src/main.cpp
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
    resources.qrc
)

add_executable(PokerGame ${SOURCES})

target_include_directories(PokerGame PRIVATE 
    src/core 
    src/ai 
    src/ui 
    src/controller 
    src/utils
    src/network
)

target_link_libraries(PokerGame PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network Qt6::WebSockets)

if(NOT WIN32 AND QRENCODE_LIBRARY)
    target_include_directories(PokerGame PRIVATE ${QRENCODE_INCLUDE_DIR})
    target_link_libraries(PokerGame PRIVATE ${QRENCODE_LIBRARY})
    target_compile_definitions(PokerGame PRIVATE HAS_QRENCODE)
endif()

if(WIN32)
    set_target_properties(PokerGame PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
