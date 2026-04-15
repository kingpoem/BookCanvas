cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/.cmake/)
include(Version)

project(BookCanvas VERSION ${VERSION_SEMANTIC} LANGUAGES CXX)

if(APPLE AND NOT IOS)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/Applications" CACHE PATH
      "Directory under which BookCanvas.app is installed" FORCE)
  endif()
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOMOC_PARALLEL OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(CMAKE_PREFIX_PATH C:/Qt/qt6/6.7.0/msvc2019_64 CACHE PATH "QT6 DIR" FORCE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(VERSION_RC_PATH ${CMAKE_SOURCE_DIR}/BookCanvas.rc)
    set(WIN32_MANIFEST_PATH ${CMAKE_SOURCE_DIR}/app.manifest)
    configure_file(
      ${CMAKE_SOURCE_DIR}/.cmake/version_exe.rc.in
      ${VERSION_RC_PATH}
    )
    configure_file(
      ${CMAKE_SOURCE_DIR}/.cmake/InstallerScript.iss.in
      ${CMAKE_SOURCE_DIR}/action-cli/InstallerScript.iss
    )
endif()

if(APPLE)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${IDENTIFIER})
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT})

    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${spectral_VERSION})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${spectral_VERSION})

    set(ICON_SOURCE_PATH "${CMAKE_SOURCE_DIR}/res/icon/app.icns")
    set(${PROJECT_NAME}_MAC_ICON "${ICON_SOURCE_PATH}")
    set(MACOSX_BUNDLE_ICON_FILE "app.icns")
    set_property(SOURCE "${${PROJECT_NAME}_MAC_ICON}" PROPERTY
        MACOSX_PACKAGE_LOCATION Resources)
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets Concurrent)

# 须匹配 src 下任意深度的源码（如 src/utils、src/view/component）
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

qt_add_executable(BookCanvas MACOSX_BUNDLE
    ${${PROJECT_NAME}_MAC_ICON}
    ${PROJECT_SOURCES}
    res/res.qrc
    res/icon/app.rc
    ${VERSION_RC_PATH}
    ${WIN32_MANIFEST_PATH}
)

# Add ElaWidgetTools, use static library
set(ELAWIDGETTOOLS_BUILD_STATIC_LIB "STATIC" CACHE STRING "Build ElaWidgetTools as a static library")
add_subdirectory(3rdpart/ElaWidgetTools)

# booksim2：统一在 3rdpart/booksim2/src 下用顶层 Makefile 本地编译（需 GNU Make、flex、bison、C++ 编译器）
set(BOOKSIM2_SRC_DIR ${CMAKE_SOURCE_DIR}/3rdpart/booksim2/src)
if(WIN32)
    set(BOOKSIM2_EXECUTABLE ${BOOKSIM2_SRC_DIR}/booksim.exe)
else()
    set(BOOKSIM2_EXECUTABLE ${BOOKSIM2_SRC_DIR}/booksim)
endif()

set(BOOKSIM2_MAKE_PROGRAM "" CACHE STRING
    "用于编译 booksim2 的 make 可执行文件（留空则自动查找 mingw32-make、make、gmake）")
if(BOOKSIM2_MAKE_PROGRAM)
    set(_booksim2_make_cmd "${BOOKSIM2_MAKE_PROGRAM}")
else()
    find_program(_booksim2_make_cmd NAMES mingw32-make make gmake)
endif()

if(NOT _booksim2_make_cmd)
    message(
        FATAL_ERROR
        "未找到用于构建 booksim2 的 make（已尝试 mingw32-make、make、gmake）。\n"
        "  Windows：请安装 MSYS2，执行 pacman 安装 mingw-w64 工具链、flex、bison、make，"
        "并将 ucrt64\\bin 加入 PATH，或配置 -DBOOKSIM2_MAKE_PROGRAM=... 指向 mingw32-make。\n")
endif()

add_custom_command(
    OUTPUT ${BOOKSIM2_EXECUTABLE}
    COMMAND ${CMAKE_COMMAND} -E chdir ${BOOKSIM2_SRC_DIR} ${_booksim2_make_cmd}
    COMMENT "Building booksim2 in ${BOOKSIM2_SRC_DIR}..."
    VERBATIM
)

# Add custom target
add_custom_target(booksim2
    DEPENDS ${BOOKSIM2_EXECUTABLE}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    3rdpart/ElaWidgetTools/src/include
)

target_link_libraries(BookCanvas PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Concurrent
    ElaWidgetTools
    version::version
)
if(WIN32)
  target_link_libraries(BookCanvas PRIVATE dwmapi)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.BookCanvas)
endif()
set_target_properties(BookCanvas PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS BookCanvas
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
