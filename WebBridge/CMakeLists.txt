project(WebBridge)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

# 生成可执行文件 (最终跑起来的 .exe)
add_executable(${PROJECT_NAME} ${CPP_FILES} ${HEADER_FILES})

# ⭐ 关键：链接 Qt 库 以及我们自己写的 IMCore 库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt5::Core 
    Qt5::Network 
    Qt5::WebSockets
    IMCore
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)