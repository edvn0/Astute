#include "pch/CorePCH.hpp"

#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include <GLFW/glfw3.h>

#include "core/Verify.hpp"

#include "Swapchain.inl"

namespace Engine::Graphics {

auto
recompute_size(const GLFWwindow* window)
{
  Core::BasicExtent<Core::i32> size;
  glfwGetFramebufferSize(
    const_cast<GLFWwindow*>(window), &size.width, &size.height);
  return size.as<Core::u32>();
}

auto
choose_swap_extent(const GLFWwindow* window,
                   const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    Core::i32 width{};
    Core::i32 height{};
    glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &width, &height);

    VkExtent2D actual_extent = {
      static_cast<Core::u32>(width),
      static_cast<Core::u32>(height),
    };

    actual_extent.width = std::clamp(actual_extent.width,
                                     capabilities.minImageExtent.width,
                                     capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height,
                                      capabilities.minImageExtent.height,
                                      capabilities.maxImageExtent.height);

    return actual_extent;
  }
}

auto
Swapchain::initialise(const Window* window, VkSurfaceKHR surf) -> void
{
  surface = surf;
  const auto& physical_device = Device::the().physical();

  auto graphics_queue = Device::the().get_family(QueueType::Graphics);
  queue_node_index = graphics_queue;

  Core::u32 format_count{};
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device, surface, &format_count, nullptr);

  std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device, surface, &format_count, surface_formats.data());

  if (format_count == 1 &&
      surface_formats.at(0).format == VK_FORMAT_UNDEFINED) {
    colour_format = VK_FORMAT_B8G8R8A8_UNORM;
    colour_space = surface_formats.at(0).colorSpace;
  } else {
    bool found_wanted_bgra8 = false;
    for (auto&& format : surface_formats) {
      if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
        colour_format = format.format;
        colour_space = format.colorSpace;
        found_wanted_bgra8 = true;
        break;
      }
    }

    if (!found_wanted_bgra8) {
      colour_format = surface_formats[0].format;
      colour_space = surface_formats[0].colorSpace;
    }
  }

  VkSurfaceCapabilitiesKHR surf_caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    physical_device, surface, &surf_caps);
  auto extent = choose_swap_extent(window->get_window(), surf_caps);
  size = { extent.width, extent.height };
}

auto
Swapchain::create(const Core::Extent& input_size, bool vsync) -> void
{
  is_vsync = vsync;
  size = input_size;

  const auto& device = Device::the().device();
  const auto& physical_device = Device::the().physical();

  VkSwapchainKHR old_swapchain = swapchain;

  VkSurfaceCapabilitiesKHR surf_caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    physical_device, surface, &surf_caps);

  auto present_mode = determine_present_mode(physical_device, surface, vsync);
  auto wanted_swapchain_images = calculate_swapchain_image_count(surf_caps);
  image_count = wanted_swapchain_images;
  auto pre_transform = select_surface_transformation(surf_caps);
  auto composite_alpha = select_composite_alpha(surf_caps);

  create_swapchain(device,
                   swapchain,
                   surface,
                   size,
                   old_swapchain,
                   present_mode,
                   wanted_swapchain_images,
                   pre_transform,
                   composite_alpha,
                   colour_space,
                   colour_format);

  for (auto& image : images)
    vkDestroyImageView(device, image.view, nullptr);
  images.clear();

  retrieve_and_store_swapchain_images(device, swapchain, vulkan_images, images);

  create_image_views(device, images, colour_format);

  for (auto& command_buffer : command_buffers)
    vkDestroyCommandPool(device, command_buffer.pool, nullptr);

  create_command_pools_and_buffers(
    device, command_buffers, queue_node_index, images.size());

  setup_semaphores(device, semaphores, image_count);

  for (auto& fence : wait_fences)
    vkDestroyFence(device, fence, nullptr);
  setup_fences(device, wait_fences, images.size());

  if (render_pass)
    vkDestroyRenderPass(device, render_pass, nullptr);
  create_render_pass(device, render_pass, colour_format);

  create_framebuffers(device, framebuffers, render_pass, images, size);
}

auto
Swapchain::destroy() -> void
{
  if (destroyed) {
    return;
  }

  auto device = Device::the().device();
  vkDeviceWaitIdle(device);

  if (swapchain)
    vkDestroySwapchainKHR(device, swapchain, nullptr);

  for (auto& image : images)
    vkDestroyImageView(device, image.view, nullptr);

  for (auto& command_buffer : command_buffers)
    vkDestroyCommandPool(device, command_buffer.pool, nullptr);

  if (render_pass)
    vkDestroyRenderPass(device, render_pass, nullptr);

  for (auto& framebuffer : framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  for (auto& semaphore : semaphores) {
    if (semaphore.render_complete)
      vkDestroySemaphore(device, semaphore.render_complete, nullptr);

    if (semaphore.present_complete)
      vkDestroySemaphore(device, semaphore.present_complete, nullptr);
  }

  for (auto& fence : wait_fences)
    vkDestroyFence(device, fence, nullptr);

  vkDeviceWaitIdle(device);

  destroyed = true;
}

Swapchain::Swapchain(const Window* window)
  : backpointer(window)
{
}

Swapchain::~Swapchain()
{
  destroy();
}

auto
Swapchain::acquire_next_image() -> Core::Maybe<Core::u32>
{
  const auto& device = Device::the().device();

  Core::u32 image_index{};
  auto result = vkAcquireNextImageKHR(
    device,
    swapchain,
    UINT64_MAX,
    semaphores.at(get_current_buffer_index()).present_complete,
    nullptr,
    &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    info("Resize from acquire.");
    on_resize(size);
    return {};
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    error("Could not acquire image");
    assert(false);
  }

  return image_index;
}

static constexpr Core::u64 DEFAULT_FENCE_TIMEOUT = 100000000000;
auto
Swapchain::begin_frame() -> bool
{

  auto acquired = acquire_next_image();
  if (!acquired)
    return false;

  current_image_index = acquired.value();

  return true;
}

auto
Swapchain::present() -> void
{
  const auto& device = Device::the().device();

  VkPipelineStageFlags wait_stage_mask =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo present_submit_info = {};
  present_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  present_submit_info.pWaitDstStageMask = &wait_stage_mask;
  present_submit_info.pWaitSemaphores =
    &semaphores.at(get_current_buffer_index()).present_complete;
  present_submit_info.waitSemaphoreCount = 1;
  present_submit_info.pSignalSemaphores =
    &semaphores.at(get_current_buffer_index()).render_complete;
  present_submit_info.signalSemaphoreCount = 1;
  present_submit_info.pCommandBuffers =
    &command_buffers.at(get_current_buffer_index()).command_buffer;
  present_submit_info.commandBufferCount = 1;

  VK_CHECK(vkResetFences(device, 1, &wait_fences[get_current_buffer_index()]));
  VK_CHECK(vkQueueSubmit(Device::the().get_queue(QueueType::Graphics),
                         1,
                         &present_submit_info,
                         wait_fences[get_current_buffer_index()]));

  VkResult result;
  {
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image_index;

    present_info.pWaitSemaphores =
      &semaphores.at(get_current_buffer_index()).render_complete;
    present_info.waitSemaphoreCount = 1;
    result = vkQueuePresentKHR(Device::the().get_queue(QueueType::Graphics),
                               &present_info);
  }

  if (result != VK_SUCCESS) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      auto new_size = recompute_size(backpointer->get_native());
      on_resize(new_size);
    } else {
      error("Failed to present swapchain image. Reason: {}",
            static_cast<Core::u32>(result));
      assert(false);
    }
  }

  current_buffer_index = (current_buffer_index + 1) % image_count;
}

auto
Swapchain::on_resize(const Core::Extent& new_size) -> void
{
  const auto& device = Device::the().device();
  vkDeviceWaitIdle(device);
  create(new_size, is_vsync);
  vkDeviceWaitIdle(device);
}

} // namespace Engine::Graphics
