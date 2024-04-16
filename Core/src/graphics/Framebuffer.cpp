#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Device.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Image.hpp"

#include "core/Verify.hpp"

namespace Engine::Graphics {

Framebuffer::Framebuffer(Configuration config)
  : size(config.size)
  , colour_attachment_formats(config.colour_attachment_formats)
  , depth_attachment_format(config.depth_attachment_format)
{
  create_colour_attachments();
  create_depth_attachment();
  create_renderpass();
  create_framebuffer();
}

Framebuffer::~Framebuffer()
{
  destroy();
}

auto
Framebuffer::destroy() -> void
{
  Allocator allocator{ "Framebuffer::~Framebuffer" };
  for (const auto& image : colour_attachments) {
    image->destroy();
  }

  depth_attachment->destroy();

  vkDestroyFramebuffer(Device::the().device(), framebuffer, nullptr);
  vkDestroyRenderPass(Device::the().device(), renderpass, nullptr);
}

auto
Framebuffer::create_colour_attachments() -> void
{
  for (auto& format : colour_attachment_formats) {
    Core::Scope<Image> image = Core::make_scope<Image>();
    create_image(size.width,
                 size.height,
                 1,
                 VK_SAMPLE_COUNT_1_BIT,
                 format,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_SAMPLED_BIT,
                 image->image,
                 image->allocation,
                 image->allocation_info);
    image->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    image->view = create_view(image->image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    image->sampler = create_sampler(VK_FILTER_LINEAR,
                                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

    auto& descriptor_info = image->descriptor_info;
    descriptor_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_info.imageView = image->view;
    descriptor_info.sampler = image->sampler;

    colour_attachments.push_back(std::move(image));
  }
}

auto
Framebuffer::create_depth_attachment() -> void
{
  if (depth_attachment_format == VK_FORMAT_UNDEFINED) {
    return;
  }

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
  image->aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
  image->view =
    create_view(image->image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
  image->sampler = create_sampler(VK_FILTER_LINEAR,
                                  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                  VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

  auto& descriptor_info = image->descriptor_info;
  descriptor_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  descriptor_info.imageView = image->view;
  descriptor_info.sampler = image->sampler;

  depth_attachment = std::move(image);
}

auto
Framebuffer::create_renderpass() -> void
{
  std::vector<VkAttachmentDescription> attachments;
  std::vector<VkAttachmentReference> colour_attachment_refs;
  VkAttachmentReference depth_attachment_ref{};
  attachments.reserve(colour_attachments.size() + 1);

  for (const auto& format : colour_attachment_formats) {
    VkAttachmentDescription attachment{};
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference ref{};
    ref.attachment = static_cast<Core::u32>(attachments.size());
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments.push_back(attachment);
    colour_attachment_refs.push_back(ref);
  }

  if (depth_attachment_format != VK_FORMAT_UNDEFINED) {
    VkAttachmentDescription attachment{};
    attachment.format = depth_attachment_format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    depth_attachment_ref.attachment =
      static_cast<Core::u32>(attachments.size());
    depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments.push_back(attachment);
  }

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount =
    static_cast<Core::u32>(colour_attachment_refs.size());
  subpass.pColorAttachments = colour_attachment_refs.data();
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency depth_dependency{};
  depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depth_dependency.dstSubpass = 0;
  depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  depth_dependency.srcAccessMask = 0;
  depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array dependencies = { dependency, depth_dependency };

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = static_cast<Core::u32>(attachments.size());
  render_pass_info.pAttachments = attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount =
    static_cast<Core::u32>(dependencies.size());
  render_pass_info.pDependencies = dependencies.data();

  VK_CHECK(vkCreateRenderPass(
    Device::the().device(), &render_pass_info, nullptr, &renderpass));

  // Create clear values
  for (const auto& format : colour_attachment_formats) {
    VkClearValue clear_value{};
    clear_value.color = { 0.0F, 0.0F, 0.0F, 1.0F };
    clear_values.push_back(clear_value);
  }

  if (depth_attachment_format != VK_FORMAT_UNDEFINED) {
    VkClearValue clear_value{};
    clear_value.depthStencil = { 1.0F, 0 };
    clear_values.push_back(clear_value);
  }
}

auto
Framebuffer::create_framebuffer() -> void
{
  std::vector<VkImageView> attachments;
  for (const auto& image : colour_attachments) {
    attachments.push_back(image->view);
  }

  if (depth_attachment_format != VK_FORMAT_UNDEFINED) {
    attachments.push_back(depth_attachment->view);
  }

  VkFramebufferCreateInfo framebuffer_info{};
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.renderPass = get_renderpass();
  framebuffer_info.attachmentCount = static_cast<Core::u32>(attachments.size());
  framebuffer_info.pAttachments = attachments.data();
  framebuffer_info.width = size.width;
  framebuffer_info.height = size.height;
  framebuffer_info.layers = 1;

  VK_CHECK(vkCreateFramebuffer(
    Device::the().device(), &framebuffer_info, nullptr, &framebuffer));
}

auto
Framebuffer::on_resize(const Core::Extent& new_size) -> void
{
  size = new_size;
  destroy();
  create_colour_attachments();
  create_depth_attachment();
  create_renderpass();
  create_framebuffer();
}

auto
Framebuffer::construct_blend_states() const
  -> std::vector<VkPipelineColorBlendAttachmentState>
{
  std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
  color_blend_attachments.reserve(colour_attachment_formats.size());

  for (const auto& format : colour_attachment_formats) {
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachments.push_back(color_blend_attachment);
  }

  return color_blend_attachments;
}

}