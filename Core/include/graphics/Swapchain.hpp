#pragma once

#include "graphics/Forward.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "core/Types.hpp"

namespace Engine::Graphics {

class Swapchain
{
public:
  explicit Swapchain(const Window*);
  ~Swapchain();

  auto destroy() -> void;
  auto initialise(const Window*, VkSurfaceKHR) -> void;
  auto create(const Core::Extent&, bool) -> void;
  auto begin_frame() -> bool;
  auto present() -> void;
  auto acquire_next_image() -> Core::Maybe<Core::u32>;

  auto on_resize(const Core::Extent&) -> void;

  [[nodiscard]] auto get_surface() const -> VkSurfaceKHR { return surface; }
  [[nodiscard]] auto get_swapchain() const -> VkSwapchainKHR
  {
    return swapchain;
  }
  [[nodiscard]] auto get_image(Core::u32 index) const -> VkImage
  {
    return images[index].image;
  }
  [[nodiscard]] auto get_image_view(Core::u32 index) const -> VkImageView
  {
    return images[index].view;
  }
  [[nodiscard]] auto get_renderpass() const -> VkRenderPass
  {
    return render_pass;
  }
  [[nodiscard]] auto get_command_buffer(Core::u32 index) const
    -> VkCommandBuffer
  {
    return command_buffers[index].command_buffer;
  }
  [[nodiscard]] auto get_current_buffer_index() const -> Core::u32
  {
    return current_buffer_index;
  }
  [[nodiscard]] auto get_current_image_index() const -> Core::u32
  {
    return current_image_index;
  }
  [[nodiscard]] auto get_size() const -> const Core::Extent& { return size; }
  [[nodiscard]] auto get_colour_format() const -> VkFormat
  {
    return colour_format;
  }
  [[nodiscard]] auto get_colour_space() const -> VkColorSpaceKHR
  {
    return colour_space;
  }
  [[nodiscard]] auto get_image_count() const -> Core::u32
  {
    return image_count;
  }

  [[nodiscard]] auto get_framebuffer(Core::u32 index) const -> VkFramebuffer
  {
    return framebuffers[index];
  }
  [[nodiscard]] auto get_framebuffer() const -> VkFramebuffer
  {
    return framebuffers[current_image_index];
  }
  [[nodiscard]] auto get_drawbuffer() const
  {
    return command_buffers[get_current_buffer_index()].command_buffer;
  }

private:
  const Window* backpointer{ nullptr };
  VkSurfaceKHR surface{};
  bool is_vsync{ false };
  bool destroyed{ false };

  VkFormat colour_format{};
  VkColorSpaceKHR colour_space{};

  VkSwapchainKHR swapchain{ nullptr };
  Core::u32 image_count = 0;
  std::vector<VkImage> vulkan_images;

  struct SwapchainImage
  {
    VkImage image{ nullptr };
    VkImageView view{ nullptr };
  };
  std::vector<SwapchainImage> images;

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
  std::vector<SwapchainSemaphores> semaphores{};

  std::vector<VkFence> wait_fences;

  VkRenderPass render_pass{ nullptr };
  Core::u32 current_buffer_index{ 0 };
  Core::u32 current_image_index{ 0 };

  Core::u32 queue_node_index = UINT32_MAX;
  Core::Extent size{};
};

} // namespace Engine::Graphics
