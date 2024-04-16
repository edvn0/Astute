#pragma once

#include <vulkan/vulkan.h>

#include "core/Types.hpp"

namespace Engine::Graphics {

class DescriptorResource
{
public:
  ~DescriptorResource();

  [[nodiscard]] auto allocate_descriptor_set(
    const VkDescriptorSetAllocateInfo& alloc_info) const -> VkDescriptorSet;
  [[nodiscard]] auto allocate_many_descriptor_sets(
    const VkDescriptorSetAllocateInfo& alloc_info) const
    -> std::vector<VkDescriptorSet>;

  void begin_frame(Core::u32 frame);
  void end_frame();

  static auto construct() -> Core::Scope<DescriptorResource>;

private:
  explicit DescriptorResource();

  void create_pool();
  void handle_fragmentation() const;
  void handle_out_of_memory() const;

  Core::u32 current_frame{ 0 };
  std::array<VkDescriptorPool, 3> descriptor_pools;
  std::array<VkDescriptorPoolSize, 11> pool_sizes;

  // Optional: Structure to keep track of allocated descriptor sets and their
  // usage
};

} // namespace Engine::Graphics
