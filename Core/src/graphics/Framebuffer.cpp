#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Device.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Image.hpp"

#include "core/Verify.hpp"

namespace Engine::Graphics {

static constexpr auto depth_formats = std::array{
  VK_FORMAT_D32_SFLOAT,         VK_FORMAT_D16_UNORM,
  VK_FORMAT_D16_UNORM_S8_UINT,  VK_FORMAT_D24_UNORM_S8_UINT,
  VK_FORMAT_D32_SFLOAT_S8_UINT,
};

static constexpr auto is_depth_format = [](VkFormat format) {
  for (const auto& fmt : depth_formats) {
    if (format == fmt)
      return true;
  }
  return false;
};

static constexpr auto find_by_format = [](const auto& container,
                                          VkFormat format) -> Core::Ref<Image> {
  auto it = std::ranges::find_if(container, [&fmt = format](const auto& kv) {
    auto&& [k, v] = kv;
    return static_cast<bool>(v) && v->format == fmt;
  });
  if (it != std::cend(container))
    return it->second;
  else
    return nullptr;
};

Framebuffer::Framebuffer(const Configuration& config)
  : size(config.size)
  , colour_attachment_formats(config.colour_attachment_formats)
  , depth_attachment_format(config.depth_attachment_format)
  , sample_count(config.sample_count)
  , resizable(config.resizable)
  , dependent_images(config.dependent_images)
  , clear_colour_on_load(config.clear_colour_on_load)
  , depth_clear_value(config.depth_clear_value)
  , name(config.name)
{
  create_colour_attachments();
  create_depth_attachment();
  if (config.immediate_construction) {
    create_framebuffer_fully();
  }

  trace("Created framebuffer: {}", name);
}

auto
Framebuffer::create_framebuffer_fully() -> void
{
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
  if (depth_attachment_index == -1)
    return nullptr;

  return dependent_images[depth_attachment_index].get();
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
    if (is_depth_format(format))
      continue;

    const auto& image = find_by_format(dependent_images, format);
    if (image) {
      colour_attachments.push_back(image);
    } else {
      Core::Ref<Image> image = Core::make_ref<Image>();
      create_image(
        size.width,
        size.height,
        1,
        sample_count,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        image->image,
        image->allocation,
        image->allocation_info,
        name + "-Colour Attachment MSAA");
      image->extent = { size.width, size.height, 1 };
      image->format = format;
      image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      image->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
      image->sample_count = sample_count;
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
  }
}

auto
Framebuffer::create_depth_attachment() -> void
{
  auto i = -1;
  auto maybe_image =
    std::ranges::find_if(dependent_images, [&i](const auto& val) {
      auto&& [k, v] = val;

      if (static_cast<bool>(v) && is_depth_format(v->format)) {
        i = k;
        return true;
      }
      return false;
    });

  const Core::Ref<Image>& image =
    maybe_image != std::cend(dependent_images) ? maybe_image->second : nullptr;

  if (image) {
    depth_attachment_format = image->format;
  }

  if (depth_attachment_format == VK_FORMAT_UNDEFINED) {
    return;
  }

  if (!image) {
    Core::Ref<Image> created_image = Core::make_ref<Image>();
    create_image(
      size.width,
      size.height,
      1,
      sample_count,
      depth_attachment_format,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      created_image->image,
      created_image->allocation,
      created_image->allocation_info,
      name + "-Depth Attachment MSAA");
    created_image->extent = { size.width, size.height, 1 };
    created_image->format = depth_attachment_format;
    created_image->sample_count = sample_count;
    created_image->layout =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    created_image->aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depth_attachment_format == VK_FORMAT_D24_UNORM_S8_UINT ||
        depth_attachment_format == VK_FORMAT_D16_UNORM_S8_UINT ||
        depth_attachment_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
      created_image->aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    created_image->view = create_view(created_image->image,
                                      depth_attachment_format,
                                      created_image->aspect_mask);
    created_image->sampler =
      create_sampler(VK_FILTER_LINEAR,
                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                     VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

    auto& descriptor_info = created_image->descriptor_info;
    descriptor_info.imageLayout =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    descriptor_info.imageView = created_image->view;
    descriptor_info.sampler = created_image->sampler;

    depth_attachment = std::move(created_image);
  } else {
    depth_attachment_index = i;
    depth_attachment = image;
  }
}

auto
Framebuffer::create_renderpass() -> void
{
  VkSubpassDescription2 subpass{};
  std::vector<VkAttachmentDescription2> attachments;
  std::vector<VkAttachmentReference2> color_attachment_refs;
  VkAttachmentReference2 depth_attachment_ref{};
  attachments.reserve(colour_attachment_formats.size() + 1);

  // Iterate over color attachment formats
  attach_colour_attachments(attachments, color_attachment_refs);
  attach_depth_attachments(attachments, depth_attachment_ref);

  std::vector<VkAttachmentReference2> resolved_attachment_refs;
  if (!resolved_attachments.empty()) {
    for (auto&& [desc, ref] : resolved_render_pass_attachments) {
      attachments.push_back(desc);
      resolved_attachment_refs.push_back(ref);
    }
  }

  VkSubpassDescriptionDepthStencilResolve depth_stencil_resolve{};
  depth_stencil_resolve.sType =
    VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
  depth_stencil_resolve.pNext = nullptr;

  subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount =
    static_cast<Core::u32>(color_attachment_refs.size());
  subpass.pColorAttachments = color_attachment_refs.data();
  subpass.pNext = nullptr;

  VkAttachmentReference2 depth_resolve_reference{};
  VkAttachmentDescription2 resolve_depth_attachment_desc{};
  if (depth_attachment != nullptr) {
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    const auto found = search_dependents_for_depth_format();

    if (!found && depth_attachment->sample_count != VK_SAMPLE_COUNT_1_BIT) {
      resolve_depth_attachment_desc.sType =
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      resolve_depth_attachment_desc.format = depth_attachment->format;
      resolve_depth_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
      resolve_depth_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      resolve_depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      resolve_depth_attachment_desc.stencilLoadOp =
        VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      resolve_depth_attachment_desc.stencilStoreOp =
        VK_ATTACHMENT_STORE_OP_DONT_CARE;
      resolve_depth_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      resolve_depth_attachment_desc.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
      attachments.push_back(resolve_depth_attachment_desc);

      depth_resolve_reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
      depth_resolve_reference.attachment =
        static_cast<Core::u32>(attachments.size() - 1);
      depth_resolve_reference.layout =
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
      depth_resolve_reference.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      if (depth_attachment->format == VK_FORMAT_D24_UNORM_S8_UINT ||
          depth_attachment->format == VK_FORMAT_D16_UNORM_S8_UINT ||
          depth_attachment->format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        depth_resolve_reference.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
      depth_stencil_resolve.depthResolveMode =
        VK_RESOLVE_MODE_SAMPLE_ZERO_BIT_KHR;
      depth_stencil_resolve.stencilResolveMode =
        VK_RESOLVE_MODE_SAMPLE_ZERO_BIT_KHR;
      depth_stencil_resolve.pDepthStencilResolveAttachment =
        &depth_resolve_reference;
      subpass.pNext = &depth_stencil_resolve;
    }
  }

  if (!resolved_attachment_refs.empty()) {
    subpass.pResolveAttachments = resolved_attachment_refs.data();
  }

  VkSubpassDependency2 dependency{};
  dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency2 depth_dependency{};
  depth_dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
  depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depth_dependency.dstSubpass = 0;
  depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  depth_dependency.srcAccessMask = 0;
  depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array dependencies = { dependency, depth_dependency };

  VkRenderPassCreateInfo2 render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
  render_pass_info.attachmentCount = static_cast<Core::u32>(attachments.size());
  render_pass_info.pAttachments = attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount =
    static_cast<Core::u32>(dependencies.size());
  render_pass_info.pDependencies = dependencies.data();

  VK_CHECK(vkCreateRenderPass2(
    Device::the().device(), &render_pass_info, nullptr, &renderpass));

  // Create clear values
  for (const auto& _ : color_attachment_refs) {
    VkClearValue clear_value{};
    clear_value.color = { 0.0F, 0.0F, 0.0F, 0.0F };
    clear_values.push_back(clear_value);
  }

  if (depth_attachment_format != VK_FORMAT_UNDEFINED) {
    VkClearValue clear_value{};
    clear_value.depthStencil = { static_cast<float>(depth_clear_value), 0 };
    clear_values.push_back(clear_value);

    if (depth_resolve_reference.attachment != 0) {
      VkClearValue clear_value{};
      clear_value.depthStencil = { static_cast<float>(depth_clear_value), 0 };
      clear_values.push_back(clear_value);
    }
  }
}

void
Framebuffer::attach_depth_attachments(
  std::vector<VkAttachmentDescription2>& attachments,
  VkAttachmentReference2& depth_attachment_ref)
{
  if (depth_attachment != nullptr) {
    const auto* found_image = search_dependents_for_depth_format();
    const bool should_clear = found_image == nullptr;

    VkAttachmentDescription2 depth_attachment_desc{};
    depth_attachment_desc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    depth_attachment_desc.format =
      found_image ? found_image->format : depth_attachment->format;
    depth_attachment_desc.samples =
      found_image ? found_image->sample_count : depth_attachment->sample_count;
    depth_attachment_desc.loadOp =
      should_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_NONE;
    depth_attachment_desc.initialLayout =
      should_clear ? VK_IMAGE_LAYOUT_UNDEFINED : found_image->layout;
    depth_attachment_desc.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    attachments.push_back(depth_attachment_desc);

    depth_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
    depth_attachment_ref.attachment =
      static_cast<uint32_t>(attachments.size() - 1);
    depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }
}

void
Framebuffer::attach_colour_attachments(
  std::vector<VkAttachmentDescription2>& attachments,
  std::vector<VkAttachmentReference2>& color_attachment_refs)
{
  for (const auto& format : colour_attachment_formats) {
    if (is_depth_format(format))
      continue;

    const auto& image = find_by_format(dependent_images, format);
    if (image) {
      VkAttachmentDescription2 color_attachment{};
      color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      color_attachment.format = image->format;
      color_attachment.samples = image->sample_count;
      color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      color_attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachments.push_back(color_attachment);

      VkAttachmentReference2 color_ref{};
      color_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
      color_ref.attachment = static_cast<uint32_t>(attachments.size() - 1);
      color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_attachment_refs.push_back(color_ref);
    } else {
      VkAttachmentDescription2 color_attachment{};
      color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
      color_attachment.format = format;
      color_attachment.samples = sample_count;
      color_attachment.loadOp = clear_colour_on_load
                                  ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                  : VK_ATTACHMENT_LOAD_OP_LOAD;
      color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      color_attachment.initialLayout =
        clear_colour_on_load ? VK_IMAGE_LAYOUT_UNDEFINED
                             : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachments.push_back(color_attachment);

      VkAttachmentReference2 color_ref{};
      color_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
      color_ref.attachment = static_cast<uint32_t>(attachments.size() - 1);
      color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_attachment_refs.push_back(color_ref);
    }
  }
}

auto
Framebuffer::create_framebuffer() -> void
{
  std::vector<VkImageView> attachments;
  for (const auto& image : colour_attachments) {
    attachments.push_back(image->view);
  }

  if (!resolved_attachments.empty()) {
    for (const auto& image : resolved_attachments) {
      attachments.push_back(image->view);
    }
  }

  if (depth_attachment) {
    attachments.push_back(depth_attachment->view);
  }

  if (resolved_depth_attachment) {
    attachments.push_back(resolved_depth_attachment->view);
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
Framebuffer::add_resolve_for_colour(Core::u32 index) -> void
{
  if (index >= colour_attachments.size()) {
    return;
  }
  const auto& image = colour_attachments[index];
  if (image->sample_count == VK_SAMPLE_COUNT_1_BIT) {
    return;
  }

  Core::Ref<Image> resolve_image = Core::make_ref<Image>();
  create_image(size.width,
               size.height,
               1,
               VK_SAMPLE_COUNT_1_BIT,
               image->format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               resolve_image->image,
               resolve_image->allocation,
               resolve_image->allocation_info,
               name + "-Resolve Attachment");
  resolve_image->extent = { size.width, size.height, 1 };
  resolve_image->format = image->format;
  resolve_image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  resolve_image->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
  resolve_image->view =
    create_view(resolve_image->image, image->format, VK_IMAGE_ASPECT_COLOR_BIT);
  resolve_image->sampler = create_sampler(VK_FILTER_LINEAR,
                                          VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                          VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

  auto& descriptor_info = resolve_image->descriptor_info;
  descriptor_info.imageLayout = resolve_image->layout;
  descriptor_info.imageView = resolve_image->view;
  descriptor_info.sampler = resolve_image->sampler;

  resolved_attachments.push_back(std::move(resolve_image));

  VkAttachmentDescription2 resolve_attachment{};
  resolve_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
  resolve_attachment.format = image->format;
  resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference2 resolve_ref{};
  resolve_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
  resolve_ref.attachment = static_cast<Core::u32>(
    colour_attachments.size() + resolved_attachments.size() - 1);
  resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  resolved_render_pass_attachments.push_back(
    std::make_pair(resolve_attachment, resolve_ref));
}

auto
Framebuffer::add_resolve_for_depth() -> void
{
  if (depth_attachment == nullptr) {
    return;
  }

  Core::Ref<Image> resolve_image = Core::make_ref<Image>();
  create_image(size.width,
               size.height,
               1,
               VK_SAMPLE_COUNT_1_BIT,
               depth_attachment->format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               resolve_image->image,
               resolve_image->allocation,
               resolve_image->allocation_info,
               name + "-Resolve Depth Attachment");
  resolve_image->extent = { size.width, size.height, 1 };
  resolve_image->format = depth_attachment->format;
  resolve_image->layout =
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
  resolve_image->aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (depth_attachment->format == VK_FORMAT_D24_UNORM_S8_UINT ||
      depth_attachment->format == VK_FORMAT_D16_UNORM_S8_UINT ||
      depth_attachment->format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
    resolve_image->aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  resolve_image->view = create_view(
    resolve_image->image, depth_attachment->format, resolve_image->aspect_mask);
  resolve_image->sampler =
    create_sampler(VK_FILTER_LINEAR,
                   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                   VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

  auto& descriptor_info = resolve_image->descriptor_info;
  descriptor_info.imageLayout =
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
  descriptor_info.imageView = resolve_image->view;
  descriptor_info.sampler = resolve_image->sampler;

  resolved_depth_attachment = std::move(resolve_image);

  VkAttachmentDescription2 resolve_attachment{};
  resolve_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
  resolve_attachment.format = depth_attachment->format;
  resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  resolve_attachment.finalLayout =
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

  auto& attachment = resolved_depth_attachment_desc.emplace();
  attachment = resolve_attachment;
}

auto
Framebuffer::construct_blend_states() const
  -> std::vector<VkPipelineColorBlendAttachmentState>
{
  std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
  color_blend_attachments.reserve(colour_attachment_formats.size());

  for (const auto& format : colour_attachment_formats) {
    if (is_depth_format(format))
      continue;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachments.push_back(color_blend_attachment);
  }

  return color_blend_attachments;
}
}
