#include "pch/CorePCH.hpp"

#include "graphics/DescriptorResource.hpp"

#include "core/Verify.hpp"
#include "graphics/Device.hpp"

namespace Engine::Graphics {

static constexpr Core::u32 frame_count = 3;

DescriptorResource::DescriptorResource()
{
  // Initialize Vulkan objects
  create_pool();
}

DescriptorResource::~DescriptorResource()
{
  // Cleanup Vulkan objects
  for (const auto& descriptor_pool : descriptor_pools) {
    vkDestroyDescriptorPool(Device::the().device(), descriptor_pool, nullptr);
  }
}

auto
DescriptorResource::allocate_descriptor_set(
  const VkDescriptorSetAllocateInfo& alloc_info) const -> VkDescriptorSet
{
  VkDescriptorSet descriptor_set = nullptr;

  // Create a allocation info copy, where we insert current pool.
  auto alloc_info_copy = alloc_info;
  alloc_info_copy.descriptorPool = descriptor_pools[current_frame];
  alloc_info_copy.descriptorSetCount = 1;

  if (const auto result = vkAllocateDescriptorSets(
        Device::the().device(), &alloc_info_copy, &descriptor_set);
      result == VK_ERROR_FRAGMENTATION_EXT) {
    handle_fragmentation();
  } else if (result == VK_ERROR_OUT_OF_POOL_MEMORY) {
    handle_out_of_memory();
  }

  return descriptor_set;
}

auto
DescriptorResource::allocate_many_descriptor_sets(
  const VkDescriptorSetAllocateInfo& alloc_info) const
  -> std::vector<VkDescriptorSet>
{
  if (alloc_info.descriptorSetCount == 1) {
    return { allocate_descriptor_set(alloc_info) };
  }

  std::vector<VkDescriptorSet> descriptor_sets(alloc_info.descriptorSetCount);
  auto alloc_info_copy = alloc_info;

  alloc_info_copy.descriptorPool = descriptor_pools[current_frame];

  VK_CHECK(vkAllocateDescriptorSets(
    Device::the().device(), &alloc_info_copy, descriptor_sets.data()));

  return descriptor_sets;
}

void
DescriptorResource::begin_frame(Core::u32 frame)
{
  current_frame = frame;
  // Cleanup or reset operations for the beginning of the frame

  vkResetDescriptorPool(
    Device::the().device(), descriptor_pools[current_frame], 0);
}

void
DescriptorResource::end_frame()
{
  // Cleanup or reset operations for the end of the frame
}

auto
DescriptorResource::construct() -> Core::Scope<DescriptorResource>
{
  return Core::Scope<DescriptorResource>(new DescriptorResource());
}

void
DescriptorResource::create_pool()
{
  pool_sizes = {
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_SAMPLER,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
      .descriptorCount = 10000 * frame_count,
    },
    VkDescriptorPoolSize{
      .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .descriptorCount = 10000 * frame_count,
    },
  };

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  // We only use two descriptor sets for now
  pool_info.maxSets = 10000 * frame_count;

  for (auto& descriptor_pool : descriptor_pools) {
    VK_CHECK(vkCreateDescriptorPool(
      Device::the().device(), &pool_info, nullptr, &descriptor_pool));
  }
}

void
DescriptorResource::handle_fragmentation() const
{
  // Handle fragmentation issues
}

void
DescriptorResource::handle_out_of_memory() const
{
  // Handle out of memory errors
}

}