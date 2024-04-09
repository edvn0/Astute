cmake_minimum_required(VERSION 3.23)

project(Application)

add_executable(App main.cpp)
target_link_libraries(App PRIVATE Core)

target_precompile_headers(App REUSE_FROM Core)
