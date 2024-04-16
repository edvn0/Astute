#pragma once

#include "core/Types.hpp"

#include "graphics/Forward.hpp"

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace Engine::Core {
class Shader;
}

namespace Engine::Compilation {

enum class DebugInformationLevel : Core::u8
{
  None = 0,
  Minimal = 1,
  Full = 2
};

struct ShaderCompilerConfiguration
{
  // Settings for the compilation settings, such as optimization levels and
  // debug information levels. The default values are:
  // - Optimization level: 0
  // - Debug information level: None
  // - Warnings as errors: false
  // - Include directories: None
  // - Macro definitions: None

  // The optimization level to use when compiling the shader.
  // The default value is 0.
  const Core::u32 optimisation_level = 0;

  // The debug information level to use when compiling the shader.
  // The default value is None.
  const DebugInformationLevel debug_information_level =
    DebugInformationLevel::None;

  // Whether to treat warnings as errors when compiling the shader.
  // The default value is false.
  const bool warnings_as_errors = false;

  // The include directories to use when compiling the shader.
  // The default value is an empty list.
  const std::vector<std::filesystem::path> include_directories = {};

  // The macro definitions to use when compiling the shader.
  // The default value is an empty list.
  const std::unordered_map<std::string, std::string> macro_definitions = {};
};

class ShaderCompiler
{
public:
  ShaderCompiler(const ShaderCompilerConfiguration& conf);
  ~ShaderCompiler();

  auto compile_graphics(const std::filesystem::path& vertex_shader_path,
                        const std::filesystem::path& fragment_shader_path)
    -> Core::Ref<Graphics::Shader>;

  auto compile_compute(const std::filesystem::path& compute_shader_path)
    -> Core::Ref<Graphics::Shader>;

  auto compile_graphics_scoped(
    const std::filesystem::path& vertex_shader_path,
    const std::filesystem::path& fragment_shader_path)
    -> Core::Scope<Graphics::Shader>;
  auto compile_compute_scoped(const std::filesystem::path& compute_shader_path)
    -> Core::Scope<Graphics::Shader>;

private:
  const ShaderCompilerConfiguration configuration;

  // Hide the implementation details of the shader compiler.
  struct Impl;
  Core::Scope<Impl> impl;
};

} // namespace Compilation
