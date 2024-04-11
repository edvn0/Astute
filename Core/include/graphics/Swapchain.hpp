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

  // clang-format off
  auto get_surface() const -> VkSurfaceKHR { return surface; }
  auto get_swapchain() const -> VkSwapchainKHR { return swapchain; }
  auto get_image(Core::u32 index) const -> VkImage { return images[index].image; }
  auto get_image_view(Core::u32 index) const -> VkImageView { return images[index].view; }
  auto get_depth_image() const -> VkImageView { return depth_stencil_image.view; }
  auto get_renderpass() const -> VkRenderPass { return render_pass; }
  auto get_command_buffer(Core::u32 index) const -> VkCommandBuffer { return command_buffers[index].command_buffer; }
  auto get_semaphore() const -> VkSemaphore { return semaphores.present_complete; }
  auto get_current_buffer_index() const -> Core::u32 { return current_buffer_index; }
  // auto get_current_image_index() const -> Core::u32 { return current_image_index; }
  auto get_size() const -> const Core::Extent& { return size; }
  auto get_colour_format() const -> VkFormat { return colour_format; }
  auto get_colour_space() const -> VkColorSpaceKHR { return colour_space; }
  auto get_image_count() const -> Core::u32 { return image_count; }

  auto get_framebuffer(Core::u32 index) const -> VkFramebuffer { return framebuffers[index]; }
  auto get_framebuffer() const -> VkFramebuffer { return framebuffers[get_current_buffer_index()]; }
  auto get_drawbuffer () const { return command_buffers[get_current_buffer_index()].command_buffer; }
  // clang-format on

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
