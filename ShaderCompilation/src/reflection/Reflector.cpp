#include "pch/CorePCH.hpp"

#include "reflection/Reflector.hpp"

#include "core/Exceptions.hpp"
#include "core/Logger.hpp"
#include "core/Verify.hpp"
#include "graphics/Shader.hpp"

#include <SPIRV-Cross/spirv_cross.hpp>
#include <SPIRV-Cross/spirv_glsl.hpp>
#include <ranges>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "reflection/ReflectionData.hpp"

namespace Engine::Reflection {

static constexpr auto to_stage = [](auto execution_model) {
  switch (execution_model) {
    case spv::ExecutionModelVertex:
      return VK_SHADER_STAGE_VERTEX_BIT;
    case spv::ExecutionModelFragment:
      return VK_SHADER_STAGE_FRAGMENT_BIT;
    case spv::ExecutionModelGLCompute:
      return VK_SHADER_STAGE_COMPUTE_BIT;
    default:
      throw std::runtime_error("Unknown execution model");
  }
};

struct Reflector::CompilerImpl
{
  std::unordered_map<Graphics::Shader::Type, Core::Scope<spirv_cross::Compiler>>
    compilers;

  CompilerImpl(
    std::unordered_map<Graphics::Shader::Type,
                       Core::Scope<spirv_cross::Compiler>>&& compiler)
    : compilers(std::move(compiler))
  {
  }
};

Reflector::Reflector(Graphics::Shader& shader)
  : shader(shader)
{
  std::array potential_types = {
    Graphics::Shader::Type::Compute,
    Graphics::Shader::Type::Vertex,
    Graphics::Shader::Type::Fragment,
  };
  std::unordered_map<Graphics::Shader::Type, Core::Scope<spirv_cross::Compiler>>
    output;
  for (const auto& type : potential_types) {
    const auto code_or = shader.get_code(type);
    if (!code_or)
      continue;

    const auto& data = code_or.value();
    const auto size = data.size();
    const auto* spirv_data = std::bit_cast<const Core::u32*>(data.data());

    // Does size need to be changed after the cast?
    const auto fixed_size = size / sizeof(Core::u32);

    output.try_emplace(
      type, Core::make_scope<spirv_cross::Compiler>(spirv_data, fixed_size));
  }

  impl = Core::make_scope<CompilerImpl>(std::move(output));
}

Reflector::~Reflector() = default;

static constexpr auto check_for_gaps = [](const auto& unordered_map) {
  if (unordered_map.empty())
    return false;

  std::int32_t max_index = 0;
  for (const auto& key : unordered_map | std::views::keys) {
    max_index = std::max(max_index, static_cast<Core::i32>(key));
  }

  for (int i = 0; i < max_index; ++i) {
    if (!unordered_map.contains(i)) {
      return true;
    }
  }

  return false;
};

namespace Detail {

auto
spir_type_to_shader_uniform_type(const spirv_cross::SPIRType& type)
  -> ShaderUniformType
{
  switch (type.basetype) {
    case spirv_cross::SPIRType::Boolean:
      return ShaderUniformType::Bool;
    case spirv_cross::SPIRType::Int:
      if (type.vecsize == 1) {
        return ShaderUniformType::Int;
      }
      if (type.vecsize == 2) {
        return ShaderUniformType::IVec2;
      }
      if (type.vecsize == 3) {
        return ShaderUniformType::IVec3;
      }
      if (type.vecsize == 4) {
        return ShaderUniformType::IVec4;
      }

    case spirv_cross::SPIRType::UInt:
      return ShaderUniformType::UInt;
    case spirv_cross::SPIRType::Float:
      if (type.columns == 3) {
        return ShaderUniformType::Mat3;
      }
      if (type.columns == 4) {
        return ShaderUniformType::Mat4;
      }

      if (type.vecsize == 1) {
        return ShaderUniformType::Float;
      }
      if (type.vecsize == 2) {
        return ShaderUniformType::Vec2;
      }
      if (type.vecsize == 3) {
        return ShaderUniformType::Vec3;
      }
      if (type.vecsize == 4) {
        return ShaderUniformType::Vec4;
      }
      break;
    default: {
      return ShaderUniformType::None;
    }
  }
  return ShaderUniformType::None;
}

std::unordered_map<Core::u32, std::unordered_map<Core::u32, UniformBuffer>>
  global_uniform_buffers{};
std::unordered_map<Core::u32, std::unordered_map<Core::u32, StorageBuffer>>
  global_storage_buffers{};

template<auto Type>
struct DescriptorTypeReflector
{
  static auto reflect(const spirv_cross::Compiler&,
                      const spirv_cross::SmallVector<spirv_cross::Resource>&,
                      ReflectionData&) -> void = delete;
};

static auto
reflect_push_constants(
  const spirv_cross::Compiler& compiler,
  const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
  ReflectionData& output) -> void
{

  for (const auto& resource : resources) {
    const auto& buffer_type = compiler.get_type(resource.base_type_id);
    const auto buffer_size =
      static_cast<Core::u32>(compiler.get_declared_struct_size(buffer_type));
    const auto member_count =
      static_cast<Core::u32>(buffer_type.member_types.size());

    Core::u32 buffer_offset = 0;
    if (!output.push_constant_ranges.empty()) {
      buffer_offset = output.push_constant_ranges.back().offset +
                      output.push_constant_ranges.back().size;
    }

    auto& push_constant_range = output.push_constant_ranges.emplace_back();
    const auto& execution_model = compiler.get_execution_model();
    push_constant_range.shader_stage = VK_SHADER_STAGE_ALL;
    push_constant_range.size = buffer_size;
    push_constant_range.offset = buffer_offset;

    const auto& buffer_name = resource.name;
    if (buffer_name.empty()) {
      continue;
    }

    auto& buffer = output.constant_buffers[buffer_name];
    buffer.name = buffer_name;
    buffer.size = buffer_size;

    for (Core::u32 i = 0; i < member_count; i++) {
      const auto& type = compiler.get_type(buffer_type.member_types[i]);
      const auto& member_name = compiler.get_member_name(buffer_type.self, i);
      auto size = static_cast<Core::u32>(
        compiler.get_declared_struct_member_size(buffer_type, i));
      auto offset = compiler.type_struct_member_offset(buffer_type, i);

      std::string uniform_name = std::format("{}.{}", buffer_name, member_name);
      buffer.uniforms[uniform_name] = ShaderUniform(
        uniform_name, spir_type_to_shader_uniform_type(type), size, offset);
    }
  }
}

template<>
struct DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>
{
  static auto reflect(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    ReflectionData& output) -> void
  {
    for (const auto& resource : resources) {
      const auto& name = resource.name;
      const auto& buffer_type = compiler.get_type(resource.base_type_id);
      auto binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
      auto descriptor_set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto size =
        static_cast<Core::u32>(compiler.get_declared_struct_size(buffer_type));

      if (descriptor_set >= output.shader_descriptor_sets.size()) {
        output.shader_descriptor_sets.resize(descriptor_set + 1);
      }

      auto& shader_descriptor_set =
        output.shader_descriptor_sets[descriptor_set];
      if (!global_uniform_buffers[descriptor_set].contains(binding)) {
        UniformBuffer uniform_buffer;
        uniform_buffer.binding_point = binding;
        uniform_buffer.size = size;
        uniform_buffer.name = name;
        uniform_buffer.shader_stage = VK_SHADER_STAGE_ALL;
        global_uniform_buffers.at(descriptor_set)[binding] = uniform_buffer;
      } else {
        if (auto& uniform_buffer =
              global_uniform_buffers.at(descriptor_set).at(binding);
            size > uniform_buffer.size) {
          uniform_buffer.size = size;
        }
      }
      shader_descriptor_set.uniform_buffers[binding] =
        global_uniform_buffers.at(descriptor_set).at(binding);
    }
  }
};

template<>
struct DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>
{
  static auto reflect(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    ReflectionData& output) -> void
  {
    for (const auto& resource : resources) {
      const auto& name = compiler.get_name(resource.base_type_id);
      const auto& buffer_type = compiler.get_type(resource.base_type_id);
      auto binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
      auto descriptor_set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto size =
        static_cast<Core::u32>(compiler.get_declared_struct_size(buffer_type));

      if (descriptor_set >= output.shader_descriptor_sets.size()) {
        output.shader_descriptor_sets.resize(descriptor_set + 1);
      }

      auto& shader_descriptor_set =
        output.shader_descriptor_sets[descriptor_set];
      if (!global_storage_buffers[descriptor_set].contains(binding)) {
        StorageBuffer storage_buffer;
        storage_buffer.binding_point = binding;
        storage_buffer.size = size;
        storage_buffer.name = name;
        storage_buffer.shader_stage = VK_SHADER_STAGE_ALL;
        global_storage_buffers.at(descriptor_set)[binding] = storage_buffer;
      } else {
        if (auto& storage_buffer =
              global_storage_buffers.at(descriptor_set).at(binding);
            size > storage_buffer.size) {
          storage_buffer.size = size;
        }
      }
      output.resources[name] = ShaderResourceDeclaration(name, binding, 1);

      shader_descriptor_set.storage_buffers[binding] =
        global_storage_buffers.at(descriptor_set).at(binding);
    }
  }
};

template<>
struct DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>
{
  static auto reflect(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    ReflectionData& output) -> void
  {
    for (const auto& resource : resources) {
      const auto& name = resource.name;
      const auto& type = compiler.get_type(resource.type_id);
      auto binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
      auto descriptor_set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto array_size = type.array[0];
      if (array_size == 0 || array_size > 16) {
        array_size = 1;
      }
      if (descriptor_set >= output.shader_descriptor_sets.size()) {
        output.shader_descriptor_sets.resize(descriptor_set + 1);
      }

      auto& shader_descriptor_set =
        output.shader_descriptor_sets[descriptor_set];
      auto& image_sampler = shader_descriptor_set.sampled_images[binding];
      image_sampler.binding_point = binding;
      image_sampler.descriptor_set = descriptor_set;
      image_sampler.name = name;
      const auto& execution_model = compiler.get_execution_model();
      image_sampler.shader_stage = to_stage(execution_model);
      image_sampler.array_size = array_size;

      output.resources[name] = ShaderResourceDeclaration(name, binding, 1);
    }
  }
};

template<>
struct DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>
{
  static auto reflect(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    ReflectionData& output) -> void
  {
    for (const auto& resource : resources) {
      const auto& name = resource.name;
      const auto& type = compiler.get_type(resource.type_id);
      auto binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
      auto descriptor_set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto array_size = type.array[0];
      if (array_size == 0 || array_size > 16) {
        array_size = 1;
      }
      if (descriptor_set >= output.shader_descriptor_sets.size()) {
        output.shader_descriptor_sets.resize(descriptor_set + 1);
      }

      auto& shader_descriptor_set =
        output.shader_descriptor_sets[descriptor_set];
      auto& image_sampler = shader_descriptor_set.storage_images[binding];
      image_sampler.binding_point = binding;
      image_sampler.descriptor_set = descriptor_set;
      image_sampler.name = name;
      image_sampler.array_size = array_size;
      const auto& execution_model = compiler.get_execution_model();
      image_sampler.shader_stage = to_stage(execution_model);

      output.resources[name] = ShaderResourceDeclaration(name, binding, 1);
    }
  }
};

template<>
struct DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>
{
  static auto reflect(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    ReflectionData& output) -> void
  {
    for (const auto& resource : resources) {
      const auto& name = resource.name;
      const auto& type = compiler.get_type(resource.type_id);
      auto binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
      auto descriptor_set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto array_size = type.array[0];
      if (array_size == 0 || array_size > 16) {
        array_size = 1;
      }
      if (descriptor_set >= output.shader_descriptor_sets.size()) {
        output.shader_descriptor_sets.resize(descriptor_set + 1);
      }

      auto& shader_descriptor_set =
        output.shader_descriptor_sets[descriptor_set];
      auto& image_sampler = shader_descriptor_set.separate_textures[binding];
      image_sampler.binding_point = binding;
      image_sampler.descriptor_set = descriptor_set;
      image_sampler.name = name;
      image_sampler.array_size = array_size;
      const auto& execution_model = compiler.get_execution_model();
      image_sampler.shader_stage = to_stage(execution_model);

      output.resources[name] = ShaderResourceDeclaration(name, binding, 1);
    }
  }
};

template<>
struct DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_SAMPLER>
{
  static auto reflect(
    const spirv_cross::Compiler& compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    ReflectionData& output) -> void
  {
    for (const auto& resource : resources) {
      const auto& name = resource.name;
      const auto& type = compiler.get_type(resource.type_id);
      auto binding =
        compiler.get_decoration(resource.id, spv::DecorationBinding);
      const auto descriptor_set =
        compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto array_size = type.array[0];
      if (array_size == 0 || array_size > 16) {
        array_size = 1;
      }
      if (descriptor_set >= output.shader_descriptor_sets.size()) {
        output.shader_descriptor_sets.resize(descriptor_set + 1);
      }

      auto& shader_descriptor_set =
        output.shader_descriptor_sets[descriptor_set];
      auto& image_sampler = shader_descriptor_set.separate_samplers[binding];
      image_sampler.binding_point = binding;
      image_sampler.descriptor_set = descriptor_set;
      image_sampler.name = name;
      image_sampler.array_size = array_size;
      const auto& execution_model = compiler.get_execution_model();
      image_sampler.shader_stage = to_stage(execution_model);

      output.resources[name] = ShaderResourceDeclaration(name, binding, 1);
    }
  }
};

} // namespace Detail

auto
reflect_on_resource(
  const auto& compiler,
  VkDescriptorType type,
  const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
  ReflectionData& reflection_data) -> void
{
  switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      Detail::DescriptorTypeReflector<
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>::reflect(compiler,
                                                    resources,
                                                    reflection_data);
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      Detail::DescriptorTypeReflector<
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>::reflect(compiler,
                                                    resources,
                                                    reflection_data);
      break;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      Detail::DescriptorTypeReflector<
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>::reflect(compiler,
                                                   resources,
                                                   reflection_data);
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      Detail::DescriptorTypeReflector<
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>::reflect(compiler,
                                                   resources,
                                                   reflection_data);
      break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      Detail::DescriptorTypeReflector<
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>::reflect(compiler,
                                                            resources,
                                                            reflection_data);
      break;
    case VK_DESCRIPTOR_TYPE_SAMPLER:
      Detail::DescriptorTypeReflector<VK_DESCRIPTOR_TYPE_SAMPLER>::reflect(
        compiler, resources, reflection_data);
      break;
    default:
      error("Unknown descriptor type: {}", (Core::u32)type);
      throw std::runtime_error("Unknown descriptor type");
  }
}

auto
Reflector::reflect(std::vector<VkDescriptorSetLayout>& output,
                   ReflectionData& reflection_data_output) const -> void
{
  for (const auto& [type, compiler] : impl->compilers) {
    // Show all the resources in the shader
    // How many sets are there? I.e. how many descriptor set layouts do we need?
    const auto& resources = compiler->get_shader_resources();

    std::unordered_map<Core::u32, std::vector<spirv_cross::Resource>>
      resources_by_set;
    // First pass is just count the max set number
    auto count_max_pass =
      [&set_resources = resources_by_set](
        const auto& comp,
        const std::pair<VkDescriptorType,
                        const spirv_cross::SmallVector<spirv_cross::Resource>&>&
          input_resources) {
        for (const auto& resource : input_resources.second) {
          const auto set =
            comp.get_decoration(resource.id, spv::DecorationDescriptorSet);

          if (set_resources.contains(set)) {
            set_resources.at(set).push_back(resource);
          } else {
            set_resources[set] = { resource };
          }
        }
      };
    const std::unordered_map<
      VkDescriptorType,
      const spirv_cross::SmallVector<spirv_cross::Resource>&>
      all_resource_small_vectors = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, resources.uniform_buffers },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, resources.storage_buffers },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, resources.separate_images },
        { VK_DESCRIPTOR_TYPE_SAMPLER, resources.separate_samplers },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, resources.storage_images },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.sampled_images },
      };

    for (const auto& small_vector : all_resource_small_vectors) {
      count_max_pass(*compiler, small_vector);
    }

#if 0
    if (check_for_gaps(resources_by_set)) {
      error("There are gaps in the descriptor sets for shader: {}",
            shader.get_name());
      throw Core::BaseException("There are gaps in the descriptor sets");
    }
#endif

    // Second pass, reflect
    for (const auto& [k, v] : all_resource_small_vectors) {
      reflect_on_resource(*compiler, k, v, reflection_data_output);
    }
    Detail::reflect_push_constants(
      *compiler, resources.push_constant_buffers, reflection_data_output);
  }
}

} // namespace Reflection
