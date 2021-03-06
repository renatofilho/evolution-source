project(aggregator-eds)

cmake_minimum_required(VERSION 2.8.9)

include(FindPkgConfig)
# Standard install paths
include(GNUInstallDirs)

pkg_check_modules(GLIB REQUIRED glib-2.0>=2.32)
pkg_check_modules(GIO REQUIRED gio-2.0>=2.32)
pkg_check_modules(EDATASERVER REQUIRED libedataserver-1.2>=3.8)
pkg_check_modules(EBOOK REQUIRED libebook-1.2)
pkg_check_modules(EBBOKBACKEND REQUIRED libebackend-1.2)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_definitions(-DGETTEXT_PACKAGE="eds-aggregator")

# Coverage tools
OPTION(ENABLE_COVERAGE "Build with coverage analysis support" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Using coverage flags")
    find_program(COVERAGE_COMMAND gcov)
    if(NOT COVERAGE_COMMAND)
        message(FATAL_ERROR "gcov command not found")
    endif()
    SET(CMAKE_C_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
    SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
    include(${CMAKE_SOURCE_DIR}/cmake/lcov.cmake)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}"
                  -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(src)
