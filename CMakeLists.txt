cmake_minimum_required(VERSION 3.16)

project(PokerGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置 Qt 路径
set(CMAKE_PREFIX_PATH "D:/QT/6.8.3/mingw_64")

find_package(Qt6 COMPONENTS Widgets Core Gui Network REQUIRED)

set(SOURCES
    src/main.cpp
    src/core/Card.cpp
    src/core/Deck.cpp
    src/core/Hand.cpp
    src/core/Player.cpp
    src/core/GameEngine.cpp
    src/ai/SimpleAI.cpp
    src/ui/MainWindow.cpp
    src/ui/PlayerWidget.cpp
    src/ui/CardWidget.cpp
    src/controller/GameController.cpp
    src/utils/Logger.cpp
    src/network/NetworkManager.cpp
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

target_link_libraries(PokerGame PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)

if(WIN32)
    set_target_properties(PokerGame PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
