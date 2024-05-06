#pragma once

#include "core/Types.hpp"

#include "core/Exceptions.hpp"
#include "core/Forward.hpp"
#include "core/Logger.hpp"

#include "compilation/ShaderCompiler.hpp"
#include "reflection/ReflectionData.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Shader
{
public:
  enum class Type : Core::u8
  {
    Compute,
    Vertex,
    Fragment,
  };
  struct PathShaderType
  {
    const std::filesystem::path path;
    const Type type;

    auto operator<=>(const PathShaderType& other) const
    {
      return type <=> other.type;
    }
    auto operator==(const PathShaderType& other) const
    {
      return type == other.type;
    }
  };

  struct Hasher
  {
    using is_transparent = void;
    auto operator()(const PathShaderType& type) const noexcept -> Core::usize
    {
      static std::hash<Core::u32> hasher;
      return hasher(static_cast<Core::u32>(type.type));
    }
  };

  explicit Shader(
    const std::unordered_set<PathShaderType, Hasher, std::equal_to<>>&);
  explicit Shader(std::unordered_map<Type, std::vector<Core::u32>>,
                  std::string_view name = "");

  ~Shader();
  auto on_resize(const Core::Extent&) -> void {}

  [[nodiscard]] auto get_shader_module(Type t = Type::Compute) const
    -> std::optional<VkShaderModule>
  {
    return shader_modules.contains(t) ? shader_modules.at(t)
                                      : std::optional<VkShaderModule>{};
  }

  [[nodiscard]] auto get_code(Type t = Type::Compute) const
    -> std::optional<std::string>
  {
    // First check parsed_spirv_per_stage_u32
    if (parsed_spirv_per_stage_u32.contains(t)) {
      std::vector<Core::u32> spirv = parsed_spirv_per_stage_u32.at(t);
      return std::string(std::bit_cast<const char*>(spirv.data()),
                         spirv.size() * sizeof(Core::u32));
    }

    return parsed_spirv_per_stage.contains(t) ? parsed_spirv_per_stage.at(t)
                                              : std::optional<std::string>{};
  }

  [[nodiscard]] auto get_descriptor_set_layouts() const -> const auto&
  {
    return descriptor_set_layouts;
  }
  [[nodiscard]] auto get_push_constant_ranges() const -> const auto&
  {
    return reflection_data.push_constant_ranges;
  }

  [[nodiscard]] auto get_name() const -> const std::string& { return name; }
  [[nodiscard]] auto get_reflection_data() const -> const auto&
  {
    return reflection_data;
  }

  [[nodiscard]] auto allocate_descriptor_set(Core::u32 set) const
    -> Reflection::MaterialDescriptorSet;

  /**
   * @brief Get the descriptor set object
   * @param descriptor_name name of a descriptor in a set
   * @param set_index the descriptor set as described by a shader
   * @return const VkWriteDescriptorSet*
   */
  [[nodiscard]] auto get_descriptor_set(std::string_view, Core::u32) const
    -> const VkWriteDescriptorSet*;

  [[nodiscard]] auto hash() const -> Core::usize;
  [[nodiscard]] auto has_descriptor_set(Core::u32 set) const -> bool;

  static auto compile_graphics(const std::filesystem::path&,
                               const std::filesystem::path&,
                               bool force_recompile = false)
    -> Core::Ref<Shader>;
  static auto compile_compute(const std::filesystem::path&)
    -> Core::Ref<Shader>;
  static auto compile_graphics_scoped(const std::filesystem::path&,
                                      const std::filesystem::path&,
                                      bool force_recompile = false)
    -> Core::Scope<Shader>;
  static auto compile_compute_scoped(const std::filesystem::path&)
    -> Core::Scope<Shader>;

  static auto initialise_compiler(
    const Compilation::ShaderCompilerConfiguration&) -> void;

private:
  std::string name{};
  Core::usize hash_value{ 0 };
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts{};
  Reflection::ReflectionData reflection_data{};
  std::unordered_map<Type, VkShaderModule> shader_modules{};
  std::unordered_map<Type, std::string> parsed_spirv_per_stage{};
  std::unordered_map<Type, std::vector<Core::u32>> parsed_spirv_per_stage_u32{};

  void create_descriptor_set_layouts();

  // Using forward declaration to avoid circular dependency of ShaderCompiler
  static inline Core::Scope<Compilation::ShaderCompiler> compiler{ nullptr };
};

} // namespace Engine::Graphics
