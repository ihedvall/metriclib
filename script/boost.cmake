# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT

include( CMakePrintHelpers )

if (NOT Boost_FOUND)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_ARCHITECTURE -x64)
    set(Boost_NO_WARN_NEW_VERSIONS ON)
    set(Boost_DEBUG OFF)
    if(POLICY CMP0167)
        cmake_policy(SET CMP0167 OLD)
    endif()

    if (COMP_DIR)
        set(Boost_ROOT ${COMP_DIR}/boost/latest)
    endif()
    find_package(Boost REQUIRED COMPONENTS container system )
endif()

cmake_print_variables( Boost_FOUND Boost_INCLUDE_DIRS Boost_LIBRARY_DIRS
                       Boost_LIBRARIES Boost_VERSION_STRING )

cmake_print_properties( TARGETS Boost::boost PROPERTIES INCLUDE_DIRECTORIES)
