cmake_minimum_required(VERSION 3.23)

project(AstuteEngine CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
    message(STATUS "CCACHE is found")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else(CCACHE_FOUND)
    message(STATUS "CCACHE is NOT found")
endif(CCACHE_FOUND)

add_subdirectory(Library)
add_subdirectory(Core)
add_subdirectory(Application)
