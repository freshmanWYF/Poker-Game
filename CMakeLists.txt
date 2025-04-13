cmake_minimum_required(VERSION 3.16)
project(PokerServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置Qt6安装路径
set(CMAKE_PREFIX_PATH "D:/QT/6.8.3/mingw_64")

# 查找Qt包
find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    REQUIRED)

# 添加源文件
set(PROJECT_SOURCES
    main.cpp
    PokerGame.cpp
    GoldenFlower.cpp
    Card.cpp
)

# 添加头文件
set(PROJECT_HEADERS
    PokerGame.h
    GoldenFlower.h
    Card.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# 设置Windows应用程序
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 复制Qt运行时依赖
if(WIN32)
    set(DEBUG_SUFFIX)
    if(MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if(NOT EXISTS "${QT_INSTALL_PATH}")
        set(QT_INSTALL_PATH "$ENV{Qt6_DIR}")
    endif()
    if(EXISTS "${QT_INSTALL_PATH}")
        # 复制Qt核心DLL
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Core>
                $<TARGET_FILE:Qt6::Gui>
                $<TARGET_FILE:Qt6::Widgets>
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
        
        # 复制Qt插件目录结构和DLL
        # 平台插件 - platforms
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms
        )
        
        # 使用Qt6::QWindowsIntegrationPlugin来获取正确的平台插件路径
        if(TARGET Qt6::QWindowsIntegrationPlugin)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/
            )
        else()
            message(WARNING "Qt6::QWindowsIntegrationPlugin target not found, trying direct path")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${QT_INSTALL_PATH}/plugins/platforms
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms
            )
        endif()
        
        # 样式插件 - styles
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/styles
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${QT_INSTALL_PATH}/plugins/styles
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/styles
        )
        
        # 图像格式插件 - imageformats
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/imageformats
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${QT_INSTALL_PATH}/plugins/imageformats
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/imageformats
        )
    endif()
endif()
