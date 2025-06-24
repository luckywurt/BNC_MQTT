# CMake 最低版本和项目名称
cmake_minimum_required(VERSION 3.14)
project(BNC)

# C++标准与Qt自动处理配置
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 安装路径（根据 Qt Creator 配置）
set(CMAKE_PREFIX_PATH "C:/ProgramData/Qt/5.15.2/mingw81_64")

# 查找 Qt 模块
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg PrintSupport Network SerialPort Concurrent OpenGL Mqtt)

# ================= 依赖库：newmat =================
file(GLOB_RECURSE NEWMAT_SRC newmat/*.cpp)
file(GLOB_RECURSE NEWMAT_HDR newmat/*.h)
add_library(newmat STATIC ${NEWMAT_SRC} ${NEWMAT_HDR})
target_include_directories(newmat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/newmat)

# ================= 依赖库：qwt =================
file(GLOB_RECURSE QWT_SRC qwt/*.cpp)
file(GLOB_RECURSE QWT_HDR qwt/*.h)
add_library(qwt STATIC ${QWT_SRC} ${QWT_HDR})
target_include_directories(qwt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qwt)
target_link_libraries(qwt PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::PrintSupport Qt5::OpenGL Qt5::Concurrent)

# ================= 依赖库：qwtpolar =================
file(GLOB_RECURSE QWTPOLAR_SRC qwtpolar/*.cpp)
file(GLOB_RECURSE QWTPOLAR_HDR qwtpolar/*.h)
add_library(qwtpolar STATIC ${QWTPOLAR_SRC} ${QWTPOLAR_HDR})
target_include_directories(qwtpolar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qwtpolar ${CMAKE_CURRENT_SOURCE_DIR}/qwt)
target_link_libraries(qwtpolar PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport Qt5::Concurrent qwt)

# ================= PPP 实现选择 =================
# 模拟 Qt Creator 中的 exists(PPP) 判断逻辑
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PPP")
    file(GLOB_RECURSE PPP_SRC src/PPP/*.cpp)
    file(GLOB_RECURSE PPP_HDR src/PPP/*.h)
    set(PPP_DEFINES USE_PPP)
    set(PPP_INC src/PPP)
else()
    file(GLOB_RECURSE PPP_SRC src/PPP_SSR_I/*.cpp)
    file(GLOB_RECURSE PPP_HDR src/PPP_SSR_I/*.h)
    set(PPP_DEFINES USE_PPP_SSR_I)
    set(PPP_INC src/PPP_SSR_I)
endif()

# ================= 主程序源文件 =================
file(GLOB_RECURSE SRC_CPP src/*.cpp)
file(GLOB_RECURSE SRC_H src/*.h)
file(GLOB_RECURSE SRC_UI src/*.ui)
file(GLOB_RECURSE SRC_QRC src/*.qrc)

qt5_add_resources(BNC_RESOURCES ${SRC_QRC})

# 如果未使用 WebEngine 模块，移除相关 UI 实现
list(REMOVE_ITEM SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/map/bncmapwin.cpp)
list(REMOVE_ITEM SRC_H   ${CMAKE_CURRENT_SOURCE_DIR}/src/map/bncmapwin.h)

# Windows 下移除 POSIX 串口实现文件
if(WIN32)
    list(REMOVE_ITEM SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/posix_qextserialport.cpp)
endif()

# 移除测试用 main 函数，防止重定义
list(REMOVE_ITEM SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/test_serial.cpp)

# 保证 PPP 模块唯一性：只保留 PPP_SRC 中的文件
file(GLOB_RECURSE PPP_ALL_CPP src/PPP/*.cpp src/PPP_SSR_I/*.cpp)
file(GLOB_RECURSE PPP_ALL_H   src/PPP/*.h   src/PPP_SSR_I/*.h)
foreach(ppp_src ${PPP_ALL_CPP})
    if(NOT "${PPP_SRC}" MATCHES "${ppp_src}")
        list(REMOVE_ITEM SRC_CPP ${ppp_src})
    endif()
endforeach()
foreach(ppp_hdr ${PPP_ALL_H})
    if(NOT "${PPP_HDR}" MATCHES "${ppp_hdr}")
        list(REMOVE_ITEM SRC_H ${ppp_hdr})
    endif()
endforeach()

# ================ 定义可执行文件 ================
add_executable(bnc
        ${SRC_CPP}
        ${SRC_H}
        ${SRC_UI}
        ${SRC_QRC}
        ${BNC_RESOURCES}
        ${PPP_SRC}
        ${PPP_HDR}
        src/MQTT/mqttClient.cpp
        src/MQTT/mqttClient.h
        src/MQTT/jsonMessage.cpp
        src/MQTT/jsonMessage.h
)

# 头文件路径配置
include_directories(
        src
        src/serial
        src/upload
        src/RTCM
        src/RTCM3
        src/RTCM3/clock_and_orbit
        src/rinex
        src/orbComp
        src/combination
        src/map
        ${PPP_INC}
        newmat
        qwt
        qwtpolar)

# 宏定义配置（SPLITBLOCK、_TTY_WIN_、PPP宏）
add_definitions(-DSPLITBLOCK -D_TTY_WIN_ -D${PPP_DEFINES})

# 链接 Qt 和第三方库
target_link_libraries(bnc
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Svg
        Qt5::PrintSupport
        Qt5::Network
        Qt5::SerialPort
        Qt5::Concurrent
        Qt5::OpenGL
        Qt5::Mqtt
        newmat
        qwt
        qwtpolar)

# 添加 Windows 平台下图标资源文件
if(WIN32)
    target_sources(bnc PRIVATE src/bnc.rc)
endif()

# ================= 复制 Qt 运行时 DLL（Release 版本） =================
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
    endif()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows.dll")
        add_custom_command(TARGET bnc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:bnc>/plugins/platforms/")
        add_custom_command(TARGET bnc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows.dll"
                "$<TARGET_FILE_DIR:bnc>/plugins/platforms/")
    endif()
    foreach(QT_LIB Core Gui Widgets Svg PrintSupport Network SerialPort Concurrent OpenGL Mqtt)
        add_custom_command(TARGET bnc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}.dll"
                "$<TARGET_FILE_DIR:bnc>")
    endforeach()
endif()

#
## 测试代码
## 添加 src/MQTT 目录中的源文件
#file(GLOB_RECURSE MQTT_SRC src/MQTT/*.cpp)
#file(GLOB_RECURSE MQTT_HDR src/MQTT/*.h)
## 创建静态库
#add_library(mqttlib STATIC ${MQTT_SRC} ${MQTT_HDR})
#target_include_directories(mqttlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/MQTT)
#target_link_libraries(mqttlib PUBLIC Qt5::Core Qt5::Mqtt )
#
## 创建测试可执行文件并链接 mqttlib
#add_executable(test_jsonmessage test/test_jsonmessage.cpp)
#target_link_libraries(test_jsonmessage PRIVATE mqttlib Qt5::Mqtt)
