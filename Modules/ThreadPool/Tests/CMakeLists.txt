cmake_minimum_required(VERSION 3.14)
project(ThreadPoolTests)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
    ThreadPoolTests
    simple_test.cpp
    command_buffer_dispatcher_test.cpp
)
target_link_libraries(
    ThreadPoolTests
    PRIVATE
    GTest::gtest_main
    ThreadPool
    Core
    VulkanMemoryAllocator
)
target_include_directories(
    ThreadPoolTests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/ThreadPool
    ${CMAKE_SOURCE_DIR}/Core
)

include(GoogleTest)
gtest_discover_tests(ThreadPoolTests)