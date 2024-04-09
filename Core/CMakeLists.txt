cmake_minimum_required(VERSION 3.23)

project(Core CXX)

add_library(Core STATIC
    include/core/Application.hpp
    src/core/Application.cpp)
target_include_directories(Core PUBLIC include)

target_precompile_headers(Core PRIVATE include/pch/CorePCH.hpp)
