if (PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(WARNING "The binary directory of CMake cannot be the same as source directory!")
endif()

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

if (WIN32)
    add_definition(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

# if (NOT MSVC)
#     find_program(CCACHE_PROGRAM ccache)
#     if (CCACHE_PROGRAM)
#         message(STATUS "~Found CCache: ${CCACHE_PROGRAM}")
#         set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
#         set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
#     endif()
# endif()

if (NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

