cmake_minimum_required(VERSION 3.23)

project(Core CXX)

add_library(Core STATIC
    include/core/Application.hpp
    src/core/Application.cpp
    include/core/Logger.hpp
    src/core/Logger.cpp
    include/core/Input.hpp
    src/core/Input.cpp
    include/graphics/Instance.hpp
    src/graphics/Instance.cpp
    include/graphics/Device.hpp
    src/graphics/Device.cpp
    include/graphics/Swapchain.hpp
    src/graphics/Swapchain.cpp
    include/graphics/Allocator.hpp
    src/graphics/Allocator.cpp
    include/core/Clock.hpp
    src/core/Clock.cpp
    include/core/Types.hpp
    include/graphics/Window.hpp
    src/graphics/Window.cpp)
target_include_directories(Core PUBLIC include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Core PUBLIC glfw Vulkan::Vulkan PRIVATE GPUOpen::VulkanMemoryAllocator)

target_precompile_headers(Core PRIVATE include/pch/CorePCH.hpp)
