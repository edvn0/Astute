#pragma once

#include "core/FrameBasedCollection.hpp"
#include "core/Types.hpp"

#include "graphics/Image.hpp"
#include "graphics/Shader.hpp"

#include "reflection/ReflectionData.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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
  auto set(std::string_view, const StorageBuffer&) -> bool;
  auto override_property(std::string_view, const Core::Ref<Image>&) -> bool;

  template<glm::length_t L, typename T, glm::qualifier Q>
  auto set(const std::string_view name, const glm::vec<L, T, Q>& vec)
  {
    set(name, glm::value_ptr(vec), L * sizeof(T));
  }

  auto set(const std::string_view name, const Core::Number auto& num)
  {
    set(name, &num, sizeof(num));
  }

  auto set(const std::string_view name, const bool& vec)
  {
    Core::PaddedBool padded{ vec };
    set(name, &padded, sizeof(padded));
  }

  [[nodiscard]] auto get_shader() const -> const auto* { return shader; }
  auto update_descriptor_write_sets(VkDescriptorSet) -> void;
  auto generate_and_update_descriptor_write_sets() -> VkDescriptorSet;

  [[nodiscard]] auto get_constant_buffer() const -> const auto&
  {
    return uniform_storage;
  }

  [[nodiscard]] auto find_image(const std::string_view name) const
  {
    for (const auto& [key, image] : images) {
      if (name == key) {
        return image;
      }
    }
    return Core::Ref<Image>{ nullptr };
  }

private:
  const Shader* shader{ nullptr };
  std::unordered_map<std::string, Core::Ref<Image>> images;
  std::unordered_map<std::string, const StorageBuffer*> storage_buffers;

  Core::FrameBasedCollection<
    std::unordered_map<Core::u32, VkWriteDescriptorSet>>
    write_descriptors;
  Core::FrameBasedCollection<Reflection::MaterialDescriptorSet> descriptor_sets;

  Core::DataBuffer uniform_storage;

  auto set(std::string_view, const void*, Core::usize) -> void;
  [[nodiscard]] auto find_resource_by_name(std::string_view) const
    -> const Reflection::ShaderResourceDeclaration*;
};

} // namespace Engine::Graphics
