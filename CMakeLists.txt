cmake_minimum_required(VERSION 3.10)

# 设置 Vcpkg 工具链 (注意路径)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty_install/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain")

project(HhheIMClient)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

# 引入 Qt 组件 (注意这里多加了 WebSockets，给 WebBridge 用的)
find_package(Qt5 REQUIRED COMPONENTS Core Network WebSockets)

# 添加子模块
add_subdirectory(IMCore)
add_subdirectory(WebBridge)