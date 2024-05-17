cmake_minimum_required(VERSION 3.21)
project(Logging)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    include/logging/Logger.hpp
    include/logging/Logger.inl
    src/logging/Logger.cpp
)
add_library(Logging STATIC ${SOURCES})
target_link_libraries(Logging PUBLIC Platform)
target_include_directories(Logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
