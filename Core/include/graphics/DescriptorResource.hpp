#pragma once

#include <vulkan/vulkan.h>

#include "core/Types.hpp"
#include <mutex>
#include <vector>

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
    if (!instance) {
      std::scoped_lock lock{ mutex };
      instance = Core::Scope<DescriptorResource>{ new DescriptorResource() };
    }
    std::scoped_lock lock{ mutex };
    return *instance;
  }

private:
  DescriptorResource() { create_pool(); }

  void create_pool();
  void handle_fragmentation() const;
  void handle_out_of_memory() const;

  Core::u32 current_frame{ 0 };
  std::vector<VkDescriptorPool> descriptor_pools;
  std::array<VkDescriptorPoolSize, 11> pool_sizes;
  static inline std::mutex mutex{};
  static inline Core::Scope<DescriptorResource> instance{};
};

} // namespace Engine::Graphics
