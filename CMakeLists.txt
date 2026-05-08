cmake_minimum_required(VERSION 3.10)

# 设置 Vcpkg 工具链 (注意路径)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty_install/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain")

project(HhheIMClient)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

# 引入 Qt 组件 (WebSockets 给 WebBridge；Widgets 给 Qt 桌面客户端)
find_package(Qt5 REQUIRED COMPONENTS Core Network WebSockets Widgets)

# 添加子模块
add_subdirectory(IMCore)
add_subdirectory(WebBridge)
add_subdirectory(QtClient)