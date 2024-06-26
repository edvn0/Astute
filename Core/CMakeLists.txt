cmake_minimum_required(VERSION 3.23)

project(Core CXX C)

add_library(Core STATIC
    include/AstuteCore.hpp/
    include/core/Application.hpp
    include/core/Camera.hpp
    include/core/Clock.hpp
    include/core/Concepts.hpp
    include/core/DataBuffer.hpp
    include/core/Event.hpp
    include/core/Exceptions.hpp
    include/core/Forward.hpp
    include/core/FrameBasedCollection.hpp
    include/core/Input.hpp
    include/core/InputCodes.hpp
    include/core/Maths.hpp
    include/core/Random.hpp
    include/core/Scene.hpp
    include/core/Types.hpp
    include/core/Verify.hpp
    include/core/Profiler.hpp
    include/graphics/Allocator.hpp
    include/graphics/CommandBuffer.hpp
    include/graphics/DescriptorResource.hpp
    include/graphics/Device.hpp
    include/graphics/Forward.hpp
    include/graphics/Framebuffer.hpp
    include/graphics/GPUBuffer.hpp
    include/graphics/Pipeline.hpp
    include/graphics/GraphicsPipeline.hpp
    include/graphics/ComputePipeline.hpp
    include/graphics/Image.hpp
    include/graphics/Instance.hpp
    include/graphics/InterfaceSystem.hpp
    include/graphics/Material.hpp
    include/graphics/Mesh.hpp
    include/graphics/RenderPass.hpp
    include/graphics/Renderer.hpp
    include/graphics/Renderer2D.hpp
    include/graphics/RendererExtensions.hpp
    include/graphics/TextureCube.hpp
    include/graphics/Shader.hpp
    include/graphics/ShaderBuffers.hpp
    include/graphics/Swapchain.hpp
    include/graphics/Vertex.hpp
    include/graphics/TextureGenerator.hpp
    include/graphics/Window.hpp
    include/graphics/render_passes/Deferred.hpp
    include/graphics/render_passes/MainGeometry.hpp
    include/graphics/render_passes/Shadow.hpp
    include/graphics/render_passes/LightCulling.hpp
    include/graphics/render_passes/Predepth.hpp
    include/graphics/render_passes/Lights.hpp
    include/graphics/render_passes/ChromaticAberration.hpp
    include/graphics/render_passes/Composition.hpp
    include/graphics/render_passes/Bloom.hpp
    include/pch/CorePCH.hpp
    include/ui/UI.hpp
    src/core/Application.cpp
    src/core/Camera.cpp
    src/core/Clock.cpp
    src/core/DataBuffer.cpp
    src/core/Input.cpp
    src/core/Random.cpp
    src/core/Scene.cpp
    src/core/Profiler.cpp
    src/graphics/Allocator.cpp
    src/graphics/CommandBuffer.cpp
    src/graphics/DescriptorResource.cpp
    src/graphics/Device.cpp
    src/graphics/Framebuffer.cpp
    src/graphics/GPUBuffer.cpp
    src/graphics/GraphicsPipeline.cpp
    src/graphics/ComputePipeline.cpp
    src/graphics/Image.cpp
    src/graphics/ImageUtilities.cpp
    src/graphics/Instance.cpp
    src/graphics/InterfaceSystem.cpp
    src/graphics/Material.cpp
    src/graphics/Mesh.cpp
    src/graphics/RenderPass.cpp
    src/graphics/Renderer.cpp
    src/graphics/Renderer2D.cpp
    src/graphics/RendererExtensions.cpp
    src/graphics/Shader.cpp
    src/graphics/Swapchain.cpp
    src/graphics/Vertex.cpp
    src/graphics/TextureCube.cpp
    src/graphics/TextureGenerator.cpp
    src/graphics/Window.cpp
    src/graphics/render_passes/Deferred.cpp
    src/graphics/render_passes/MainGeometry.cpp
    src/graphics/render_passes/Shadow.cpp
    src/graphics/render_passes/LightCulling.cpp
    src/graphics/render_passes/Predepth.cpp
    src/graphics/render_passes/Lights.cpp
    src/graphics/render_passes/ChromaticAberration.cpp
    src/graphics/render_passes/Composition.cpp
    src/graphics/render_passes/Bloom.cpp
    src/ui/UI.cpp
)
target_include_directories(Core PUBLIC include
    ${CMAKE_SOURCE_DIR}/Library/imgui
    ${CMAKE_SOURCE_DIR}/Library/glm
    ${CMAKE_SOURCE_DIR}/Library/src
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Library/assimp/include
)
target_include_directories(Core SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/Library/VulkanMemoryAllocator/include
    ${CMAKE_SOURCE_DIR}/Library/filewatch)

target_link_libraries(Core PUBLIC ThreadPool ktx glfw Vulkan::Vulkan PRIVATE VulkanMemoryAllocator imguizmo imgui glm EnTT::EnTT stbimage::stbimage assimp FastNoise2 ShaderCompilation Logging Platform)
target_precompile_headers(Core PRIVATE include/pch/CorePCH.hpp)
target_compile_definitions(Core PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE -DImTextureID=ImU64)

if("Clang" IN_LIST CMAKE_CXX_COMPILER_ID)
    target_compile_options(Core PRIVATE -Wno-unused-local-typedef)
endif()

# Add ASTUTE_PERFORMANCE as a preprocessor definition to enable performance tracking IF ASTUTE_PERFORMANCE is defined
if(ASTUTE_PERFORMANCE)
    target_compile_definitions(Core PRIVATE ASTUTE_PERFORMANCE)
endif()
