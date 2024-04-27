#pragma once

#include "core/FrameBasedCollection.hpp"
#include "core/Types.hpp"

#include "graphics/Image.hpp"
#include "graphics/Shader.hpp"

#include "reflection/ReflectionData.hpp"

#include <unordered_map>

namespace Engine::Graphics {

class Material
{
public:
  struct Configuration
  {
    const Shader* shader;
  };
  explicit Material(Configuration);
  ~Material();

  auto set(std::string_view, const Core::Ref<Image>&) -> bool;

  auto get_descriptor_set() -> decltype(auto) { return descriptor_sets.get(); }

  auto get_shader() const -> const auto* { return shader; }
  auto generate_and_update_descriptor_write_sets(VkDescriptorSet) -> void;

private:
  const Shader* shader{ nullptr };
  std::unordered_map<std::string, Core::Ref<Image>> images{};

  Core::FrameBasedCollection<
    std::unordered_map<Core::u32, VkWriteDescriptorSet>>
    write_descriptors;
  Core::FrameBasedCollection<VkDescriptorSet> descriptor_sets;

  auto find_resource_by_name(std::string_view) const
    -> const Reflection::ShaderResourceDeclaration*;
};

} // namespace Engine::Graphics
