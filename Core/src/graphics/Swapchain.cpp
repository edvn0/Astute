#include "pch/CorePCH.hpp"

#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include <GLFW/glfw3.h>

namespace Engine::Graphics {

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
  const auto& device = Device::the().device();
  const auto& physical_device = Device::the().physical();
  const auto& instance = Instance::the().instance();

  auto graphics_queue = Device::the().get_family(QueueType::Graphics);
  queue_node_index = graphics_queue;

  Core::u32 format_count{};
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device, surface, &format_count, NULL);

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
  const auto& instance = Instance::the().instance();

  VkSwapchainKHR old_swapchain = swapchain;

  VkSurfaceCapabilitiesKHR surf_caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    physical_device, surface, &surf_caps);

  auto present_mode = VK_PRESENT_MODE_FIFO_KHR;
  Core::u32 present_mode_count{};
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    physical_device, surface, &present_mode_count, nullptr);
  std::vector<VkPresentModeKHR> present_modes(present_mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    physical_device, surface, &present_mode_count, present_modes.data());

  if (!vsync) {
    for (auto i = 0; i < present_mode_count; i++) {
      if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
      if ((present_mode != VK_PRESENT_MODE_MAILBOX_KHR) &&
          (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
        present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  auto wanted_swapchain_images = surf_caps.minImageCount + 1;
  if ((surf_caps.maxImageCount > 0) &&
      (wanted_swapchain_images > surf_caps.maxImageCount)) {
    wanted_swapchain_images = surf_caps.maxImageCount;
  }

  VkSurfaceTransformFlagsKHR pre_transform{};
  if (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    pre_transform = surf_caps.currentTransform;
  }

  auto composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (auto& composite_alpha_flag : compositeAlphaFlags) {
    if (surf_caps.supportedCompositeAlpha & composite_alpha_flag) {
      composite_alpha = composite_alpha_flag;
      break;
    };
  }

  VkSwapchainCreateInfoKHR swapchain_create_info = {};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.pNext = NULL;
  swapchain_create_info.surface = surface;
  swapchain_create_info.minImageCount = wanted_swapchain_images;
  swapchain_create_info.imageFormat = colour_format;
  swapchain_create_info.imageColorSpace = colour_space;
  swapchain_create_info.imageExtent = { size.width, size.height };
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_create_info.preTransform =
    (VkSurfaceTransformFlagBitsKHR)pre_transform;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_create_info.queueFamilyIndexCount = 0;
  swapchain_create_info.pQueueFamilyIndices = NULL;
  swapchain_create_info.presentMode = present_mode;
  swapchain_create_info.oldSwapchain = old_swapchain;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.compositeAlpha = composite_alpha;

  if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain);

  if (old_swapchain)
    vkDestroySwapchainKHR(device, old_swapchain, nullptr);

  for (auto& image : images)
    vkDestroyImageView(device, image.view, nullptr);

  images.clear();

  vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
  images.resize(image_count);
  vulkan_images.resize(image_count);
  vkGetSwapchainImagesKHR(
    device, swapchain, &image_count, vulkan_images.data());

  images.resize(image_count);
  for (uint32_t i = 0; i < image_count; i++) {
    VkImageViewCreateInfo view_create_info = {};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.format = colour_format;
    view_create_info.image = vulkan_images.at(i);
    view_create_info.components = {
      VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A,
    };
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.flags = 0;

    images[i].image = vulkan_images[i];

    vkCreateImageView(device, &view_create_info, nullptr, &images[i].view);
  }

  for (auto& command_buffer : command_buffers)
    vkDestroyCommandPool(device, command_buffer.pool, nullptr);

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = queue_node_index;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType =
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = 1;

  command_buffers.resize(image_count);
  for (auto& command_buffer : command_buffers) {
    vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &command_buffer.pool);

    commandBufferAllocateInfo.commandPool = command_buffer.pool;
    vkAllocateCommandBuffers(
      device, &commandBufferAllocateInfo, &command_buffer.command_buffer);
  }

  if (!semaphores.render_complete || !semaphores.present_complete) {
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(
      device, &semaphore_create_info, nullptr, &semaphores.render_complete);
    vkCreateSemaphore(
      device, &semaphore_create_info, nullptr, &semaphores.present_complete);
  }

  if (wait_fences.size() != image_count) {
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    wait_fences.resize(image_count);
    for (auto& fence : wait_fences) {
      vkCreateFence(device, &fence_create_info, nullptr, &fence);
    }
  }

  VkPipelineStageFlags pipeline_stage_flags =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pWaitDstStageMask = &pipeline_stage_flags;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &semaphores.present_complete;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &semaphores.render_complete;

  // Render Pass
  VkAttachmentDescription colour_attachment_description = {};
  // Color attachment
  colour_attachment_description.format = colour_format;
  colour_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
  colour_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_attachment_description.stencilStoreOp =
    VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_reference = {};
  color_reference.attachment = 0;
  color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_description = {};
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.colorAttachmentCount = 1;
  subpass_description.pColorAttachments = &color_reference;
  subpass_description.inputAttachmentCount = 0;
  subpass_description.pInputAttachments = nullptr;
  subpass_description.preserveAttachmentCount = 0;
  subpass_description.pPreserveAttachments = nullptr;
  subpass_description.pResolveAttachments = nullptr;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &colour_attachment_description;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass_description;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass);

  for (auto& framebuffer : framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  VkFramebufferCreateInfo framebuffer_create_info = {};
  framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_create_info.renderPass = render_pass;
  framebuffer_create_info.attachmentCount = 1;
  framebuffer_create_info.width = size.width;
  framebuffer_create_info.height = size.height;
  framebuffer_create_info.layers = 1;

  framebuffers.resize(image_count);
  for (uint32_t i = 0; i < framebuffers.size(); i++) {
    framebuffer_create_info.pAttachments = &images[i].view;
    vkCreateFramebuffer(
      device, &framebuffer_create_info, nullptr, &framebuffers[i]);
  }
}

auto
Swapchain::destroy() -> void
{
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

  for (auto framebuffer : framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  if (semaphores.render_complete)
    vkDestroySemaphore(device, semaphores.render_complete, nullptr);

  if (semaphores.present_complete)
    vkDestroySemaphore(device, semaphores.present_complete, nullptr);

  for (auto& fence : wait_fences)
    vkDestroyFence(device, fence, nullptr);

  vkDeviceWaitIdle(device);
}

Swapchain::~Swapchain()
{
  destroy();
}

auto
Swapchain::acquire_next_image() -> Core::u32
{
  const auto& device = Device::the().device();

  Core::u32 image_index{};
  vkAcquireNextImageKHR(device,
                        swapchain,
                        UINT64_MAX,
                        semaphores.present_complete,
                        nullptr,
                        &image_index);
  return image_index;
}

auto
Swapchain::begin_frame() -> void
{
  // auto& queue =
  // Renderer::GetRenderResourceReleaseQueue(m_CurrentBufferIndex);
  // queue.Execute();
  const auto& device = Device::the().device();

  current_image_index = acquire_next_image();

  vkResetCommandPool(device, command_buffers[current_buffer_index].pool, 0);
}

auto
Swapchain::present() -> void
{
  const auto& device = Device::the().device();
  const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

  VkPipelineStageFlags wait_stage_mask =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pWaitDstStageMask = &wait_stage_mask;
  submitInfo.pWaitSemaphores = &semaphores.present_complete;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphores.render_complete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pCommandBuffers =
    &command_buffers[current_buffer_index].command_buffer;
  submitInfo.commandBufferCount = 1;

  vkResetFences(device, 1, &wait_fences[current_buffer_index]);
  vkQueueSubmit(Device::the().get_queue(QueueType::Graphics),
                1,
                &submitInfo,
                wait_fences[current_buffer_index]);

  VkResult result;
  {
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image_index;

    present_info.pWaitSemaphores = &semaphores.render_complete;
    present_info.waitSemaphoreCount = 1;
    result = vkQueuePresentKHR(Device::the().get_queue(QueueType::Graphics),
                               &present_info);
  }

  if (result != VK_SUCCESS) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      on_resize(size);
    } else {
      // TODO: Inform
      throw;
    }
  }

  {
    static constexpr auto frames_in_flight = 3ULL;
    current_buffer_index = (current_buffer_index + 1) % frames_in_flight;
    vkWaitForFences(
      device, 1, &wait_fences[current_buffer_index], VK_TRUE, UINT64_MAX);
  }
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
