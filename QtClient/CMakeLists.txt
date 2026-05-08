project(QtClient)

set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME}
	main.cpp
	MainWindow.cpp
	MainWindow.h
	ChatBubbleRow.cpp
	ChatBubbleRow.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	Qt5::Core
	Qt5::Widgets
	Qt5::Network
	IMCore
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/IMCore/include
)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
