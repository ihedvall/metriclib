# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(MqttMetricLib
        VERSION 1.0
        DESCRIPTION "Simple MQTT metric library"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_DEBUG_POSTFIX d)

set(MQTT_HEADERS)

add_library(mqtt-metric-lib
        ../include/mqtt/mqttnode.h src/mqttnode.cpp
        ../include/mqtt/mqtttopic.h src/mqtttopic.cpp
        ../include/mqtt/detectbroker.h src/detectbroker.cpp
)

target_include_directories(mqtt-metric-lib PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
target_include_directories(mqtt-metric-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(mqtt-metric-lib PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(mqtt-metric-lib PUBLIC ${PAHO_C_INCLUDE_DIRS})
target_include_directories(mqtt-metric-lib PRIVATE ${json-lib_SOURCE_DIR}/include)
cmake_print_properties(TARGETS mqtt-metric-lib PROPERTIES INCLUDE_DIRECTORIES)

if (MSVC)
    target_compile_definitions(mqtt-metric-lib PRIVATE _WIN32_WINNT=0x0A00)
endif ()

target_link_libraries(mqtt-metric-lib PRIVATE nlohmann_json::nlohmann_json)