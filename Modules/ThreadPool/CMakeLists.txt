cmake_minimum_required(VERSION 3.21)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    include/thread_pool/ThreadPool.hpp
    src/thread_pool/ThreadPool.cpp
)
add_library(ThreadPool STATIC ${SOURCES})
target_link_libraries(ThreadPool Vulkan::Vulkan)
target_include_directories(ThreadPool PUBLIC
    ${CMAKE_SOURCE_DIR}/Library
    ${CMAKE_SOURCE_DIR}/Core/include
    ${CMAKE_SOURCE_DIR}/Library/thread-pool/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE
    ${CMAKE_SOURCE_DIR}/Library/glm
)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(Tests)
endif()
