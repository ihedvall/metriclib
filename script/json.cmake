# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT

include (FetchContent)
include(CMakePrintHelpers)

FetchContent_Declare(json-lib
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG HEAD
)
FetchContent_MakeAvailable(json-lib)
cmake_print_variables({json-lib_POPULATED json-lib_SOURCE_DIR json-lib_BINARY_DIR)

cmake_print_properties(TARGETS nlohmann_json::nlohmann_json
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LIBRARIES INTERFACE_LINK_LIBRARIES )
