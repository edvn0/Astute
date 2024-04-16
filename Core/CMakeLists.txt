cmake_minimum_required(VERSION 3.23)

project(Core CXX)

add_library(Core STATIC
    include/core/Application.hpp
    include/core/Camera.hpp
    include/core/Clock.hpp
    include/core/Input.hpp
    include/core/Logger.hpp
    include/core/Platform.hpp
    include/core/Scene.hpp
    include/core/Types.hpp
    include/graphics/Allocator.hpp
    include/graphics/CommandBuffer.hpp
    include/graphics/Device.hpp
    include/graphics/Framebuffer.hpp
    include/graphics/GPUBuffer.hpp
    include/graphics/GraphicsPipeline.hpp
    include/graphics/Image.hpp
    include/graphics/Instance.hpp
    include/graphics/InterfaceSystem.hpp
    include/graphics/Renderer.hpp
    include/graphics/RendererExtensions.hpp
    include/graphics/Shader.hpp
    include/graphics/Swapchain.hpp
    include/graphics/DescriptorResource.hpp
    include/graphics/Window.hpp
    include/ui/UI.hpp
    src/core/Application.cpp
    src/core/Camera.cpp
    src/core/Clock.cpp
    src/core/Input.cpp
    src/core/Logger.cpp
    src/core/Platform.cpp
    src/core/Scene.cpp
    src/graphics/Allocator.cpp
    src/graphics/CommandBuffer.cpp
    src/graphics/Device.cpp
    src/graphics/Framebuffer.cpp
    src/graphics/GPUBuffer.cpp
    src/graphics/GraphicsPipeline.cpp
    src/graphics/Image.cpp
    src/graphics/Instance.cpp
    src/graphics/InterfaceSystem.cpp
    src/graphics/Renderer.cpp
    src/graphics/RendererExtensions.cpp
    src/graphics/Shader.cpp
    src/graphics/Swapchain.cpp
    src/graphics/Window.cpp
    src/graphics/DescriptorResource.cpp
    src/ui/UI.cpp
)
target_include_directories(Core PUBLIC include ${CMAKE_SOURCE_DIR}/Library/imgui ${CMAKE_SOURCE_DIR}/Library/glm ${CMAKE_SOURCE_DIR}/Library/src PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Core PUBLIC ShaderCompilation glfw Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator PRIVATE imgui glm EnTT::EnTT)

target_precompile_headers(Core PRIVATE include/pch/CorePCH.hpp)
