# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
include(CMakePrintHelpers)

if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    set(USE_VCPKG OFF)
else()
    set(USE_VCPKG ON)
endif()

option(BUILD_SHARED_LIBS "Static libraries are preferred" OFF)
option(METRIC_DOC "If doxygen is installed, then build documentation in Release mode" OFF)
option(METRIC_TOOLS "Building applications" OFF)
option(METRIC_TEST "Building unit test" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(METRIC_TEST AND USE_VCPKG)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()
if(METRIC_TOOLS AND USE_VCPKG)
    list(APPEND VCPKG_MANIFEST_FEATURES "tools")
endif()

project(metriclib
        VERSION 1.0
        DESCRIPTION "Interface against a metrics"
        LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_DEBUG_POSTFIX d)

if (METRIC_TOOLS)
    include(script/wxwidgets.cmake)
endif()

if (METRIC_TEST)
    include(script/googletest.cmake)
endif()

if (METRIC_DOC)
    include(script/doxygen.cmake)
endif()


set(METRIC_HEADERS
    include/metric/metric.h
    include/metric/metrictype.h
    include/metric/metricproperty.h
)

add_library(metric-lib
        src/metric.cpp include/metric/metric.h
        src/metricproperty.cpp include/metric/metricproperty.h
        include/metric/metrictype.h
        src/metrichelper.cpp
        src/metrichelper.h
        src/metricgroup.cpp
        include/metric/metricgroup.h
        src/metricdatabase.cpp
        include/metric/metricdatabase.h
)

target_include_directories(metric-lib PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
       )
target_include_directories(metric-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

cmake_print_properties(TARGETS metric-lib PROPERTIES INCLUDE_DIRECTORIES)

if (MSVC)
    target_compile_definitions(metric-lib PRIVATE _WIN32_WINNT=0x0A00)
endif ()

if (METRIC_TEST)
    enable_testing()
    add_subdirectory(test)
endif ()

if (DOXYGEN_FOUND AND METRIC_DOC)
    set(DOXYGEN_RECURSIVE NO)
    set(DOXYGEN_REPEAT_BRIEF NO)
    set(DOXYGEN_PROJECT_NAME "MetricLib")
    set(DOXYGEN_HTML_EXTRA_STYLESHEET doxygen/metriclib.css)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/manual)
    doxygen_add_docs(metric-doc ALL
            include/metric doxygen
            COMMENT "Metric Library generating API documentation with Doxygen")
endif ()


