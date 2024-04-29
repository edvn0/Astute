namespace Engine::Graphics {
auto
determine_present_mode(const VkPhysicalDevice& physical_device,
                       const VkSurfaceKHR& surface,
                       bool vsync) -> VkPresentModeKHR
{
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
  return present_mode;
}

// Calculate desired swapchain image count
auto
calculate_swapchain_image_count(const VkSurfaceCapabilitiesKHR& surf_caps)
  -> Core::u32
{
  auto wanted_swapchain_images = surf_caps.minImageCount + 1;
  if ((surf_caps.maxImageCount > 0) &&
      (wanted_swapchain_images > surf_caps.maxImageCount)) {
    wanted_swapchain_images = surf_caps.maxImageCount;
  }
  return wanted_swapchain_images;
}

// Select surface transformation
auto
select_surface_transformation(const VkSurfaceCapabilitiesKHR& surf_caps)
  -> VkSurfaceTransformFlagBitsKHR
{
  if (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    return surf_caps.currentTransform;
  }
}

// Select composite alpha flag
auto
select_composite_alpha(const VkSurfaceCapabilitiesKHR& surf_caps)
  -> VkCompositeAlphaFlagBitsKHR
{
  std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (const auto& composite_alpha_flag : composite_alpha_flags) {
    if (surf_caps.supportedCompositeAlpha & composite_alpha_flag) {
      return composite_alpha_flag;
    };
  }
  return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Default if none matched
}

auto
create_swapchain(const VkDevice& device,
                 VkSwapchainKHR& swapchain,
                 const VkSurfaceKHR& surface,
                 const Core::Extent& size,
                 VkSwapchainKHR& old_swapchain,
                 VkPresentModeKHR present_mode,
                 Core::u32 wanted_swapchain_images,
                 VkSurfaceTransformFlagBitsKHR pre_transform,
                 VkCompositeAlphaFlagBitsKHR composite_alpha,
                 VkColorSpaceKHR colour_space,
                 VkFormat colour_format) -> void
{
  VkSwapchainCreateInfoKHR swapchain_create_info{};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.surface = surface;
  swapchain_create_info.minImageCount = wanted_swapchain_images;
  swapchain_create_info.imageFormat = colour_format;
  swapchain_create_info.imageColorSpace = colour_space;
  swapchain_create_info.imageExtent = { size.width, size.height };
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_create_info.preTransform = pre_transform;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_create_info.presentMode = present_mode;
  swapchain_create_info.oldSwapchain = old_swapchain;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.compositeAlpha = composite_alpha;

  vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain);
  if (old_swapchain) {
    vkDestroySwapchainKHR(device, old_swapchain, nullptr);
    old_swapchain = nullptr;
  }
}

auto
retrieve_and_store_swapchain_images(const VkDevice& device,
                                    VkSwapchainKHR& swapchain,
                                    std::vector<VkImage>& vulkan_images,
                                    auto& images) -> void
{
  Core::u32 image_count;
  vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
  vulkan_images.resize(image_count);
  vkGetSwapchainImagesKHR(
    device, swapchain, &image_count, vulkan_images.data());

  images.resize(image_count);
  for (Core::u32 i = 0; i < image_count; i++) {
    images[i].image = vulkan_images[i];
  }
}

auto
create_image_views(const VkDevice& device, auto& images, VkFormat colour_format)
  -> void
{
  for (auto& image : images) {
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image.image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = colour_format;
    view_create_info.components = { VK_COMPONENT_SWIZZLE_R,
                                    VK_COMPONENT_SWIZZLE_G,
                                    VK_COMPONENT_SWIZZLE_B,
                                    VK_COMPONENT_SWIZZLE_A };
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &view_create_info, nullptr, &image.view);
  }
}

auto
create_command_pools_and_buffers(const VkDevice& device,
                                 auto& command_buffers,
                                 Core::u32 queue_family_index,
                                 Core::usize image_count) -> void
{
  VkCommandPoolCreateInfo cmd_pool_info{};
  cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  cmd_pool_info.queueFamilyIndex = queue_family_index;

  command_buffers.resize(image_count);
  for (auto& command_buffer : command_buffers) {
    vkCreateCommandPool(device, &cmd_pool_info, nullptr, &command_buffer.pool);

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info{};
    cmd_buffer_alloc_info.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = command_buffer.pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(
      device, &cmd_buffer_alloc_info, &command_buffer.command_buffer);
  }
}

auto
setup_semaphores(const VkDevice& device,
                 auto& semaphores_vector,
                 const auto image_count) -> void
{
  semaphores_vector.resize(image_count);
  for (auto& semaphores : semaphores_vector) {
    if (!semaphores.render_complete || !semaphores.present_complete) {
      VkSemaphoreCreateInfo semaphore_create_info{};
      semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      if (!semaphores.render_complete) {
        vkCreateSemaphore(
          device, &semaphore_create_info, nullptr, &semaphores.render_complete);
      }

      if (!semaphores.present_complete) {
        vkCreateSemaphore(device,
                          &semaphore_create_info,
                          nullptr,
                          &semaphores.present_complete);
      }
    }
  }
}

auto
setup_fences(const VkDevice& device,
             std::vector<VkFence>& wait_fences,
             Core::usize image_count) -> void
{
  VkFenceCreateInfo fence_create_info{};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  wait_fences.resize(image_count);
  for (auto& fence : wait_fences) {
    vkCreateFence(device, &fence_create_info, nullptr, &fence);
  }
}

auto
prepare_submit_info(VkSubmitInfo& submit_info,
                    const auto& semaphores,
                    const VkPipelineStageFlags& pipeline_stage_flags) -> void
{
  submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pWaitDstStageMask = &pipeline_stage_flags;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &semaphores.present_complete;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &semaphores.render_complete;
}

auto
create_render_pass(const VkDevice& device,
                   VkRenderPass& render_pass,
                   VkFormat colour_format) -> void
{
  VkAttachmentDescription colour_attachment{};
  colour_attachment.format = colour_format;
  colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colour_attachment_ref{};
  colour_attachment_ref.attachment = 0;
  colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colour_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &colour_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass);
}

auto
create_framebuffers(const VkDevice& device,
                    std::vector<VkFramebuffer>& framebuffers,
                    const VkRenderPass& render_pass,
                    auto& images,
                    const Core::Extent& size) -> void
{
  for (auto& framebuffer : framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  framebuffers.resize(images.size());

  for (Core::usize i = 0; i < images.size(); i++) {
    auto attachments = std::array{ images[i].view };

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount =
      static_cast<Core::u32>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = size.width;
    framebuffer_info.height = size.height;
    framebuffer_info.layers = 1;

    vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]);
  }
}
}
