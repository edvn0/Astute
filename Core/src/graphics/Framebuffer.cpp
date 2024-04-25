#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Device.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Image.hpp"

#include "core/Verify.hpp"

namespace Engine::Graphics {

static constexpr auto depth_formats = std::array{
  VK_FORMAT_D32_SFLOAT,
  VK_FORMAT_D16_UNORM_S8_UINT,
  VK_FORMAT_D24_UNORM_S8_UINT,
  VK_FORMAT_D32_SFLOAT_S8_UINT,
};

Framebuffer::Framebuffer(const Configuration& config)
  : size(config.size)
  , colour_attachment_formats(config.colour_attachment_formats)
  , depth_attachment_format(config.depth_attachment_format)
  , sample_count(config.sample_count)
  , resizable(config.resizable)
  , dependent_attachments(config.dependent_attachments)
  , name(config.name)
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
Framebuffer::search_dependents_for_depth_format() -> const Image*
{
  if (dependent_attachments.empty())
    return nullptr;

  const auto found =
    std::ranges::find_if(dependent_attachments, [](auto& image) {
      for (const auto& fmt : depth_formats) {
        if (fmt == image->format) {
          return true;
        }
      }
      return false;
    });

  return found != dependent_attachments.end() ? found->get() : nullptr;
}

auto
Framebuffer::destroy() -> void
{
  if (depth_attachment) {
    depth_attachment->destroy();
    depth_attachment.reset();
  }

  for (auto& image : colour_attachments) {
    image->destroy();
  }

  clear_values.clear();
  colour_attachments.clear();

  vkDestroyFramebuffer(Device::the().device(), framebuffer, nullptr);
  vkDestroyRenderPass(Device::the().device(), renderpass, nullptr);
}

auto
Framebuffer::create_colour_attachments() -> void
{
  for (auto& format : colour_attachment_formats) {
    if (sample_count != VK_SAMPLE_COUNT_1_BIT) {
      Core::Ref<Image> image = Core::make_ref<Image>();
      create_image(size.width,
                   size.height,
                   1,
                   sample_count,
                   format,
                   VK_IMAGE_TILING_OPTIMAL,
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT,
                   image->image,
                   image->allocation,
                   image->allocation_info,
                   name + "-Colour Attachment MSAA");
      image->format = format;
      image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      image->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
      image->view =
        create_view(image->image, format, VK_IMAGE_ASPECT_COLOR_BIT);
      image->sampler = create_sampler(VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

      auto& descriptor_info = image->descriptor_info;
      descriptor_info.imageLayout = image->layout;
      descriptor_info.imageView = image->view;
      descriptor_info.sampler = image->sampler;

      colour_attachments.push_back(std::move(image));
    }

    Core::Ref<Image> resolve_image = Core::make_ref<Image>();
    create_image(size.width,
                 size.height,
                 1,
                 VK_SAMPLE_COUNT_1_BIT,
                 format,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_SAMPLED_BIT,
                 resolve_image->image,
                 resolve_image->allocation,
                 resolve_image->allocation_info,
                 name + "-Colour Attachment (1 Sample)");
    resolve_image->format = format;
    resolve_image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resolve_image->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    resolve_image->view =
      create_view(resolve_image->image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    resolve_image->sampler =
      create_sampler(VK_FILTER_LINEAR,
                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                     VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

    resolve_image->descriptor_info.imageLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resolve_image->descriptor_info.imageView = resolve_image->view;
    resolve_image->descriptor_info.sampler = resolve_image->sampler;
    colour_attachments.push_back(std::move(resolve_image));
  }
}

auto
Framebuffer::create_depth_attachment() -> void
{

  if (depth_attachment_format == VK_FORMAT_UNDEFINED &&
      dependent_attachments.empty()) {
    return;
  }

  if (depth_attachment_format != VK_FORMAT_UNDEFINED &&
      dependent_attachments.empty()) {
    // Valid, create from 'depth_attachment_format'.
  }

  if (depth_attachment_format == VK_FORMAT_UNDEFINED &&
      !dependent_attachments.empty()) {
    // Possibly valid, search for a depth format among 'dependent_attachments'.
    depth_attachment_format = search_dependents_for_depth_format()->format;
  }

  if (depth_attachment_format != VK_FORMAT_UNDEFINED &&
      !dependent_attachments.empty()) {
    // Possibly valid, search for a depth format among 'dependent_attachments'.
    // IDEA: Maybe first search, and if not found, choose the format.
    auto* found = search_dependents_for_depth_format();
    if (found) {
      depth_attachment_format = found->format;
    }
  }

  auto depth_image_iterator =
    std::ranges::find_if(dependent_attachments, [](auto& image) {
      for (const auto& fmt : depth_formats) {
        if (fmt == image->format) {
          return true;
        }
      }
      return false;
    });
  auto image = depth_image_iterator != dependent_attachments.end()
                 ? *depth_image_iterator
                 : nullptr;

  if (!image) {
    Core::Ref<Image> created_image = Core::make_ref<Image>();
    create_image(size.width,
                 size.height,
                 1,
                 sample_count,
                 depth_attachment_format,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_SAMPLED_BIT,
                 created_image->image,
                 created_image->allocation,
                 created_image->allocation_info);
    created_image->format = depth_attachment_format;
    created_image->layout =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    created_image->aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    created_image->view = create_view(
      created_image->image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
    created_image->sampler =
      create_sampler(VK_FILTER_LINEAR,
                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                     VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

    auto& descriptor_info = created_image->descriptor_info;
    descriptor_info.imageLayout =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    descriptor_info.imageView = created_image->view;
    descriptor_info.sampler = created_image->sampler;

    depth_attachment = std::move(created_image);
  } else {
    depth_attachment = image;
  }
}

auto
Framebuffer::create_renderpass() -> void
{
  std::vector<VkAttachmentDescription> attachments;
  std::vector<VkAttachmentReference> color_attachment_refs;
  std::vector<VkAttachmentReference> resolve_attachment_refs;
  VkAttachmentReference depth_attachment_ref{};
  attachments.reserve(colour_attachment_formats.size() + 1);

  // Iterate over color attachment formats
  for (const auto& format : colour_attachment_formats) {
    // Always add a basic color attachment
    VkAttachmentDescription color_attachment{};
    color_attachment.format = format;
    color_attachment.samples = sample_count;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments.push_back(color_attachment);

    VkAttachmentReference color_ref{};
    color_ref.attachment = static_cast<uint32_t>(attachments.size() - 1);
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_refs.push_back(color_ref);

    // Add a resolve attachment only when MSAA is used
    if (sample_count != VK_SAMPLE_COUNT_1_BIT) {
      VkAttachmentDescription resolve_attachment{};
      resolve_attachment.format = format;
      resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachments.push_back(resolve_attachment);

      VkAttachmentReference resolve_ref{};
      resolve_ref.attachment = static_cast<uint32_t>(attachments.size() - 1);
      resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      resolve_attachment_refs.push_back(resolve_ref);
    }
  }

  // Depth attachment if needed
  if (depth_attachment != nullptr) {
    const auto* found_image = search_dependents_for_depth_format();
    const bool should_clear = found_image == nullptr;

    VkAttachmentDescription depth_attachment_desc{};
    depth_attachment_desc.format = depth_attachment_format;
    depth_attachment_desc.samples = sample_count;
    depth_attachment_desc.loadOp =
      should_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_desc.initialLayout =
      should_clear ? VK_IMAGE_LAYOUT_UNDEFINED : found_image->layout;
    depth_attachment_desc.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments.push_back(depth_attachment_desc);

    depth_attachment_ref.attachment =
      static_cast<uint32_t>(attachments.size() - 1);
    depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount =
    static_cast<Core::u32>(color_attachment_refs.size());
  subpass.pColorAttachments = color_attachment_refs.data();
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  if (const auto has_msaa = !resolve_attachment_refs.empty()) {
    subpass.pResolveAttachments = resolve_attachment_refs.data();
  }

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
  for (const auto& _ : color_attachment_refs) {
    VkClearValue clear_value{};
    clear_value.color = { 0.0F, 0.0F, 0.0F, 0.0F };
    clear_values.push_back(clear_value);
  }

  for (const auto& _ : resolve_attachment_refs) {
    VkClearValue clear_value{};
    clear_value.color = { 0.0F, 0.0F, 0.0F, 0.0F };
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
  if (resizable) {
    size = new_size;
  }

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

  for (const auto& _ : colour_attachment_formats) {
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
