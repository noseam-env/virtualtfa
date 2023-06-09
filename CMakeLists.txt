cmake_minimum_required(VERSION 3.12)

project(virtualtfa)

option(VIRTUALTFA_BUILD_STATIC "BUILD STATIC LIBRARIES" ON)
option(VIRTUALTFA_BUILD_SHARED "BUILD SHARED LIBRARIES" ON)

set(CMAKE_CXX_STANDARD 17)

set(VIRTUALTFA_SOURCES
        src/file_util.cpp
        src/file_util.hpp
        src/virtualtfa.cpp
        )

set(VIRTUALTFA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (VIRTUALTFA_BUILD_STATIC)
    add_library(virtualtfa_static STATIC ${VIRTUALTFA_SOURCES})
    target_include_directories(virtualtfa_static PUBLIC ${VIRTUALTFA_INCLUDE_DIR})
endif ()
if (VIRTUALTFA_BUILD_SHARED)
    add_library(virtualtfa_shared SHARED ${VIRTUALTFA_SOURCES})
    target_include_directories(virtualtfa_shared PUBLIC ${VIRTUALTFA_INCLUDE_DIR})
endif ()
