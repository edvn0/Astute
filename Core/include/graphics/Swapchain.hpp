#pragma once

#include "graphics/Forward.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "core/Types.hpp"

namespace Engine::Graphics {

class Swapchain
{
public:
  Swapchain() = default;
  ~Swapchain();

  auto destroy() -> void;
  auto initialise(const Window*, VkSurfaceKHR) -> void;
  auto create(const Core::Extent&, bool) -> void;
  auto begin_frame() -> void;
  auto present() -> void;
  auto acquire_next_image() -> Core::u32;

  auto on_resize(const Core::Extent&) -> void;

private:
  VkSurfaceKHR surface;
  bool is_vsync{ false };

  VkFormat colour_format;
  VkColorSpaceKHR colour_space;

  VkSwapchainKHR swapchain{ nullptr };
  Core::u32 image_count = 0;
  std::vector<VkImage> vulkan_images;

  struct SwapchainImage
  {
    VkImage image{ nullptr };
    VkImageView view{ nullptr };
  };
  std::vector<SwapchainImage> images;

  struct DepthImage
  {
    VkImage image{ nullptr };
    VmaAllocation allocation{ nullptr };
    VkImageView view{ nullptr };
  };
  DepthImage depth_stencil_image;

  std::vector<VkFramebuffer> framebuffers;

  struct SwapchainCommandBuffer
  {
    VkCommandPool pool{ nullptr };
    VkCommandBuffer command_buffer{ nullptr };
  };
  std::vector<SwapchainCommandBuffer> command_buffers;

  struct SwapchainSemaphores
  {
    // Swap chain
    VkSemaphore present_complete{ nullptr };
    // Command buffer
    VkSemaphore render_complete{ nullptr };
  };
  SwapchainSemaphores semaphores;
  VkSubmitInfo submit_info;

  std::vector<VkFence> wait_fences;

  VkRenderPass render_pass{ nullptr };
  Core::u32 current_buffer_index{ 0 };
  Core::u32 current_image_index{ 0 };

  Core::u32 queue_node_index = UINT32_MAX;
  Core::Extent size{};
};

} // namespace Engine::Graphics
