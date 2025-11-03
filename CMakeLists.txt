cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/.cmake/)
include(Version)

project(BookCanvas VERSION ${VERSION_SEMANTIC} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

    set(ICON_NAME "res/icon/app.icns")
    set(${PROJECT_NAME}_MAC_ICON "${CMAKE_SOURCE_DIR}/${ICON_NAME}")
    set(MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
    set_property(SOURCE "${${PROJECT_NAME}_MAC_ICON}" PROPERTY
        MACOSX_PACKAGE_LOCATION Resources)
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets Concurrent)

file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
    src/*.h
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

# Set booksim2 source directory and binary directory
set(BOOKSIM2_SRC_DIR ${CMAKE_SOURCE_DIR}/3rdpart/booksim2/src)
set(BOOKSIM2_EXECUTABLE ${BOOKSIM2_SRC_DIR}/booksim)

# Custom command to compile booksim
find_program(MAKE_PROGRAM make)
if(WIN32)
    find_program(MAKE_PROGRAM mingw32-make)
    if(NOT MAKE_PROGRAM)
        find_program(MAKE_PROGRAM nmake)
    endif()
endif()

add_custom_command(
    OUTPUT ${BOOKSIM2_EXECUTABLE}
    COMMAND ${CMAKE_COMMAND} -E chdir ${BOOKSIM2_SRC_DIR} ${MAKE_PROGRAM}
    COMMENT "Building booksim2..."
    WORKING_DIRECTORY ${BOOKSIM2_SRC_DIR}
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
