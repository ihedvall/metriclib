# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(TestMetric
        VERSION 1.0
        DESCRIPTION "Google unit tests for the Metric library"
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(metric-test
        src/test_metric.cpp
        src/test_metrichelper.cpp
        src/test_metricproperty.cpp
        src/test_metricgroup.cpp
        src/test_metricdatabase.cpp
        src/test_topic.cpp
        src/test_detectbroker.cpp
        src/test_metriclogstream.cpp
        src/test_iso8601.cpp
        src/test_metricnode.cpp
        src/testmqtt.cpp
        src/testmqtt.h
        src/test_metriclisten.cpp

)

target_include_directories(metric-test PRIVATE ../src )
target_include_directories(metric-test PRIVATE ${Boost_INCLUDE_DIRS} )

if (MSVC)
    target_compile_definitions(metric-test PRIVATE -D_WIN32_WINNT=0x0A00)
endif ()


target_link_libraries(metric-test PRIVATE metric-lib)
target_link_libraries(metric-test PRIVATE mqtt-metric-lib)
target_link_libraries(metric-test PRIVATE eclipse-paho-mqtt-c::paho-mqtt3a-static)
target_link_libraries(metric-test PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(metric-test)


