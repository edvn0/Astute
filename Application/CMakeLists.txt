cmake_minimum_required(VERSION 3.23)

project(Application)

add_executable(App main.cpp AstuteApplication.cpp AstuteApplication.hpp)
target_link_libraries(App PRIVATE Core EnTT::EnTT)
target_include_directories(App PRIVATE ${CMAKE_SOURCE_DIR}/Core ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(App REUSE_FROM Core)
