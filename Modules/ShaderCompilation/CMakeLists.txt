cmake_minimum_required(VERSION 3.21)
project(ShaderCompilation)

set(SOURCES
    include/compilation/ShaderCompiler.hpp
    src/compilation/ShaderCompiler.cpp
    include/reflection/ReflectionData.hpp
    include/reflection/Reflector.hpp
    src/reflection/Reflector.cpp
)
add_library(ShaderCompilation STATIC ${SOURCES})
target_link_libraries(ShaderCompilation Vulkan::Vulkan spirv-cross-core spirv-cross-glsl Vulkan::shaderc_combined Logging)
target_include_directories(ShaderCompilation PUBLIC
    ${CMAKE_SOURCE_DIR}/Library
    ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE
    ${CMAKE_SOURCE_DIR}/Core
    ${CMAKE_SOURCE_DIR}/Core/include
    ${CMAKE_SOURCE_DIR}/Library/glm)
