project(IMCore)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

# 关键：生成静态库 (STATIC) 而不是可执行文件
add_library(${PROJECT_NAME} STATIC ${CPP_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Network)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)