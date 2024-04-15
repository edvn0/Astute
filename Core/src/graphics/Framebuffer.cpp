#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Device.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Image.hpp"

namespace Engine::Graphics {

Framebuffer::Framebuffer(Configuration config)
  : size(config.size)
{
  create_colour_attachments();
  create_depth_attachment();
  create_renderpass();
  create_framebuffer();
}

Framebuffer::~Framebuffer()
{
  Allocator allocator{ "Framebuffer::~Framebuffer" };
  for (auto& image : colour_attachments) {
    image->destroy();
  }

  depth_attachment->destroy();

  vkDestroyFramebuffer(Device::the().device(), framebuffer, nullptr);
  vkDestroyRenderPass(Device::the().device(), renderpass, nullptr);
}

auto
Framebuffer::create_colour_attachments() -> void
{
  Core::Scope<Image> image = Core::make_scope<Image>();
  create_image(size.width,
               size.height,
               1,
               VK_SAMPLE_COUNT_1_BIT,
               VK_FORMAT_B8G8R8A8_SRGB,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               image->image,
               image->allocation,
               image->allocation_info);

  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image = image->image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = VK_FORMAT_B8G8R8A8_SRGB;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  if (vkCreateImageView(
        Device::the().device(), &view_create_info, nullptr, &image->view) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create image view");
  }

  VkSamplerCreateInfo sampler_create_info{};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  // sampler_create_info.anisotropyEnable = VK_TRUE;
  // sampler_create_info.maxAnisotropy = 16;
  sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.minLod = 0;
  sampler_create_info.maxLod = 1;
  sampler_create_info.mipLodBias = 0;

  if (vkCreateSampler(Device::the().device(),
                      &sampler_create_info,
                      nullptr,
                      &image->sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create sampler");
  }

  colour_attachments.push_back(std::move(image));
}

auto
Framebuffer::create_depth_attachment() -> void
{
  Core::Scope<Image> image = Core::make_scope<Image>();
  create_image(size.width,
               size.height,
               1,
               VK_SAMPLE_COUNT_1_BIT,
               VK_FORMAT_D32_SFLOAT,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               image->image,
               image->allocation,
               image->allocation_info);

  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image = image->image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = VK_FORMAT_D32_SFLOAT;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  if (vkCreateImageView(
        Device::the().device(), &view_create_info, nullptr, &image->view) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create image view");
  }

  VkSamplerCreateInfo sampler_create_info{};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  // sampler_create_info.anisotropyEnable = VK_TRUE;
  // sampler_create_info.maxAnisotropy = 16;
  sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.minLod = 0;
  sampler_create_info.maxLod = 1;
  sampler_create_info.mipLodBias = 0;

  if (vkCreateSampler(Device::the().device(),
                      &sampler_create_info,
                      nullptr,
                      &image->sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create sampler");
  }

  depth_attachment = std::move(image);
}

auto
Framebuffer::create_renderpass() -> void
{
  VkAttachmentDescription colour_attachment_description{};
  colour_attachment_description.format = VK_FORMAT_B8G8R8A8_SRGB;
  colour_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
  colour_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_attachment_description.stencilStoreOp =
    VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_attachment_description.finalLayout =
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depth_attachment_description{};
  depth_attachment_description.format = VK_FORMAT_D32_SFLOAT;
  depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment_description.stencilStoreOp =
    VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment_description.finalLayout =
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colour_attachment_ref{};
  colour_attachment_ref.attachment = 0;
  colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout =
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colour_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {
    colour_attachment_description,
    depth_attachment_description,
  };

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
  render_pass_info.pAttachments = attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  if (vkCreateRenderPass(
        Device::the().device(), &render_pass_info, nullptr, &renderpass) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass");
  }

  // Create clear values
  clear_values.resize(2);
  clear_values[0].color = { 0.0F, 0.0F, 0.0F, 1.0F };
  clear_values[1].depthStencil = { 1.0F, 0 };
}

auto
Framebuffer::create_framebuffer() -> void
{
  std::array<VkImageView, 2> attachments = {
    colour_attachments[0]->view,
    depth_attachment->view,
  };

  VkFramebufferCreateInfo framebuffer_info{};
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.renderPass = get_renderpass();
  framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
  framebuffer_info.pAttachments = attachments.data();
  framebuffer_info.width = size.width;
  framebuffer_info.height = size.height;
  framebuffer_info.layers = 1;

  if (vkCreateFramebuffer(
        Device::the().device(), &framebuffer_info, nullptr, &framebuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create framebuffer");
  }
}

}