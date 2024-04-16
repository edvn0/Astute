#pragma once

#include <vulkan/vulkan.h>

#include "core/Types.hpp"
#include <mutex>

namespace Engine::Graphics {

class DescriptorResource
{
public:
  ~DescriptorResource();

  DescriptorResource(const DescriptorResource&) = delete;
  DescriptorResource& operator=(const DescriptorResource&) = delete;

  [[nodiscard]] auto allocate_descriptor_set(
    const VkDescriptorSetAllocateInfo& alloc_info) const -> VkDescriptorSet;
  [[nodiscard]] auto allocate_many_descriptor_sets(
    const VkDescriptorSetAllocateInfo& alloc_info) const
    -> std::vector<VkDescriptorSet>;

  void begin_frame();
  void end_frame();

  auto destroy() -> void;

  static auto the() -> DescriptorResource&
  {
    static DescriptorResource instance;
    return instance;
  }

private:
  DescriptorResource() { create_pool(); }

  void create_pool();
  void handle_fragmentation() const;
  void handle_out_of_memory() const;

  Core::u32 current_frame{ 0 };
  std::array<VkDescriptorPool, 3> descriptor_pools;
  std::array<VkDescriptorPoolSize, 11> pool_sizes;
};

} // namespace Engine::Graphics
