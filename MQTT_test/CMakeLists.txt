cmake_minimum_required(VERSION 3.14)
project(simplemqttclient LANGUAGES CXX)

# 设置 C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 .ui 文件、moc、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt 安装路径（根据实际安装位置）
set(CMAKE_PREFIX_PATH "C:/ProgramData/Qt/5.15.2/mingw81_64")

# 查找 Qt 模块
find_package(Qt5 REQUIRED COMPONENTS Core Gui Network Mqtt Widgets)

# 包含头文件和源文件
set(SOURCES
        main.cpp
        mainwindow.cpp
)

set(HEADERS
        mainwindow.h
)

set(UIS
        mainwindow.ui
)

# 添加可执行程序
add_executable(simplemqttclient
        ${SOURCES}
        ${HEADERS}
        ${UIS}
)

# 链接 Qt 库
target_link_libraries(simplemqttclient
        Qt5::Core
        Qt5::Gui
        Qt5::Network
        Qt5::Mqtt
        Qt5::Widgets
)

# DLL 路径设置
set(QT_BIN_DIR "C:/ProgramData/Qt/5.15.2/mingw81_64/bin")
set(MINGW_BIN_DIR "C:/ProgramData/Qt/Tools/mingw810_64/bin")
set(QT_PLATFORM_PLUGIN "${QT_BIN_DIR}/../plugins/platforms/qwindows.dll")

# 构建后复制 DLL 和平台插件
add_custom_command(TARGET simplemqttclient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QT_BIN_DIR}/Qt5Core.dll
        ${QT_BIN_DIR}/Qt5Gui.dll
        ${QT_BIN_DIR}/Qt5Widgets.dll
        ${QT_BIN_DIR}/Qt5Network.dll
        ${QT_BIN_DIR}/Qt5Mqtt.dll
        ${MINGW_BIN_DIR}/libgcc_s_seh-1.dll
        ${MINGW_BIN_DIR}/libstdc++-6.dll
        ${MINGW_BIN_DIR}/libwinpthread-1.dll
        $<TARGET_FILE_DIR:simplemqttclient>
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:simplemqttclient>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QT_PLATFORM_PLUGIN}
        $<TARGET_FILE_DIR:simplemqttclient>/platforms
)

