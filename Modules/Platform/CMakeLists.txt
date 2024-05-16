cmake_minimum_required(VERSION 3.21)
project(Platform)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    include/platform/Platform.hpp
)

if(WIN32)
    list(APPEND SOURCES
        impl/windows/platform/Platform.cpp
    )
else()
    # Error
    message(STATUS "Unsupported platform, including dummy implementation.")
    list(APPEND SOURCES
        impl/empty/platform/Platform.cpp
    )
endif(WIN32)

add_library(Platform STATIC ${SOURCES})
target_include_directories(Platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
