cmake_minimum_required(VERSION 4.0)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/.cmake/)
include(platform)

project(BookCanvas LANGUAGES CXX)

file(GLOB_RECURSE BOOK_CANVAS_SOURCES CONFIGURE_DEPENDS src/app/*.cpp)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

add_executable(${PROJECT_NAME} ${BOOK_CANVAS_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC Qt6::Core Qt6::Widgets
)

