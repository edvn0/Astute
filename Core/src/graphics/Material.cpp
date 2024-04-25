#include "pch/CorePCH.hpp"

#include "graphics/Material.hpp"

#include "core/Application.hpp"
#include "graphics/Device.hpp"

#include <ranges>

namespace Engine::Graphics {

Material::~Material() = default;

Material::Material(Configuration config)
  : shader(config.shader)
{
}

auto
Material::set(const std::string_view name,
              TextureType type,
              const Core::Ref<Image>& image) -> bool
{
  if (!image) {
    return false;
  }
  const auto* resource = find_resource_by_name(name);
  if (!resource) {
    return false;
  }

  images[type] = image;

  write_descriptors.for_each([&](const auto&, auto& container) {
    if (!container.contains(resource->get_register())) {
      auto& desc = container[resource->get_register()];
      desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      desc.descriptorCount = 1;
      desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      desc.dstBinding = resource->get_register();
      desc.pImageInfo = &images[type]->descriptor_info;
    }
  });

  return true;
}

auto
Material::generate_and_update_descriptor_write_sets(VkDescriptorSet dst) -> void
{
  auto& current_writes = *write_descriptors;

  for (auto& [index, write] : current_writes) {
    write.dstSet = dst;
  }

  std::vector<VkWriteDescriptorSet> values{};
  values.reserve(current_writes.size());
  for (const auto& [index, write] : current_writes) {
    values.push_back(write);
  }

  vkUpdateDescriptorSets(Device::the().device(),
                         static_cast<Core::u32>(values.size()),
                         values.data(),
                         0,
                         nullptr);
}

auto
Material::find_resource_by_name(const std::string_view name) const
  -> const Reflection::ShaderResourceDeclaration*
{
  for (const auto& [key, value] : shader->get_reflection_data().resources) {
    if (key == name) {
      return &value;
    }
  }

  return nullptr;
}

} // namespace Engine::Graphics
