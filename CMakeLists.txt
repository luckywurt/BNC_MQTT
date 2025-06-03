cmake_minimum_required(VERSION 3.14)
project(BNC)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置 Qt 安装路径（根据实际路径修改）
set(CMAKE_PREFIX_PATH "C:/ProgramData/Qt/5.15.2/mingw81_64")

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg PrintSupport Network SerialPort Concurrent OpenGL)

# ================= newmat =================
file(GLOB_RECURSE NEWMAT_SRC newmat/*.cpp)
file(GLOB_RECURSE NEWMAT_HDR newmat/*.h)
add_library(newmat STATIC ${NEWMAT_SRC} ${NEWMAT_HDR})
target_include_directories(newmat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/newmat)

# ================= qwt =================
file(GLOB_RECURSE QWT_SRC qwt/*.cpp)
file(GLOB_RECURSE QWT_HDR qwt/*.h)
add_library(qwt STATIC ${QWT_SRC} ${QWT_HDR})
target_include_directories(qwt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qwt)
target_link_libraries(qwt PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::PrintSupport Qt5::OpenGL Qt5::Concurrent)

# ================= qwtpolar =================
file(GLOB_RECURSE QWTPOLAR_SRC qwtpolar/*.cpp)
file(GLOB_RECURSE QWTPOLAR_HDR qwtpolar/*.h)
add_library(qwtpolar STATIC ${QWTPOLAR_SRC} ${QWTPOLAR_HDR})
target_include_directories(qwtpolar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qwtpolar ${CMAKE_CURRENT_SOURCE_DIR}/qwt)
target_link_libraries(qwtpolar PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport Qt5::Concurrent qwt)

# ================= PPP 或 PPP_SSR_I =================
option(USE_PPP "Use src/PPP instead of src/PPP_SSR_I" ON)

if(USE_PPP)
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

# ================= 主程序 src =================
file(GLOB_RECURSE SRC_CPP src/*.cpp)
file(GLOB_RECURSE SRC_H src/*.h)
file(GLOB_RECURSE SRC_UI src/*.ui)
file(GLOB_RECURSE SRC_QRC src/*.qrc)

qt5_add_resources(BNC_RESOURCES ${SRC_QRC})

# 如果未安装 WebEngine 模块，移除 bncmapwin 中相关引用
list(REMOVE_ITEM SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/map/bncmapwin.cpp)
list(REMOVE_ITEM SRC_H ${CMAKE_CURRENT_SOURCE_DIR}/src/map/bncmapwin.h)

# 如果在 Windows 下构建，移除 POSIX 特有的串口实现文件
if(WIN32)
    list(REMOVE_ITEM SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/posix_qextserialport.cpp)
endif()

# 排除测试用的 main 函数（如 test_serial.cpp）避免重复定义
list(REMOVE_ITEM SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/test_serial.cpp)

# ✅ 强制只包含一个 PPP 实现，移除另一个路径中所有 .cpp 和 .h
file(GLOB_RECURSE PPP_ALL_CPP src/PPP/*.cpp src/PPP_SSR_I/*.cpp)
file(GLOB_RECURSE PPP_ALL_H src/PPP/*.h src/PPP_SSR_I/*.h)
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

add_executable(bnc
        ${SRC_CPP}
        ${SRC_H}
        ${SRC_UI}
        ${SRC_QRC}
        ${BNC_RESOURCES}
        ${PPP_SRC}
        ${PPP_HDR}
)

# 资源和头文件路径
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
        qwtpolar
)

# 宏定义
add_definitions(-DSPLITBLOCK -D_TTY_WIN_ -D${PPP_DEFINES})

# 链接 Qt 和子模块库
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
        newmat
        qwt
        qwtpolar
)

# Windows 下添加 .rc 图标资源
if(WIN32)
    target_sources(bnc PRIVATE src/bnc.rc)
endif()

# ============== 复制 Qt 运行时插件（强制使用 Release DLL） ==============
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX "")
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
    foreach(QT_LIB Core Gui Widgets Svg PrintSupport Network SerialPort Concurrent OpenGL)
        add_custom_command(TARGET bnc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}.dll"
                "$<TARGET_FILE_DIR:bnc>")
    endforeach()
endif()
