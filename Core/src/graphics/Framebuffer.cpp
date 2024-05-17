#include "core/Types.hpp"
#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Image.hpp"

#include "core/Verify.hpp"
#include <algorithm>

namespace Engine::Graphics {

namespace Utils {
static constexpr auto depth_formats = std::array{
  VK_FORMAT_D32_SFLOAT,         VK_FORMAT_D16_UNORM,
  VK_FORMAT_D16_UNORM_S8_UINT,  VK_FORMAT_D24_UNORM_S8_UINT,
  VK_FORMAT_D32_SFLOAT_S8_UINT,
};

static constexpr auto is_depth_format = [](VkFormat format) {
  return std::ranges::any_of(depth_formats,
                             [=](auto val) { return format == val; });
};
}

Framebuffer::Framebuffer(const FramebufferSpecification& specification)
  : config(specification)
{
  size = {
    static_cast<Core::u32>(static_cast<Core::f32>(config.width) * config.scale),
    static_cast<Core::u32>(static_cast<Core::f32>(config.height) *
                           config.scale),
  };

  if (!size.valid()) {
    throw std::runtime_error("What!!!");
  }

  if (config.existing_framebuffer) {
    return;
  }

  Core::u32 attachment_index = 0;
  for (auto& attachment_specification : config.attachments) {
    if (config.existing_image && config.existing_image->get_layer_count() > 1) {
      if (Utils::is_depth_format(attachment_specification.format)) {
        depth_attachment_image = config.existing_image;
      } else {
        attachment_images.emplace_back(config.existing_image);
      }
    } else if (config.existing_images.contains(attachment_index)) {
      if (!Utils::is_depth_format(attachment_specification.format)) {
        attachment_images.emplace_back(); // This will be set later
      }
    } else if (Utils::is_depth_format(attachment_specification.format)) {
      const VkImageUsageFlags usage =
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      const auto name = std::format(
        "{0}-DepthAttachment{1}",
        config.debug_name.empty() ? "Unnamed FB" : config.debug_name,
        attachment_index);
      depth_attachment_image = Core::make_ref<Image>(ImageConfiguration{
        .width = scaled_width(),
        .height = scaled_height(),
        .sample_count = config.samples,
        .format = attachment_specification.format,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .usage = usage,
        .additional_name_data = name,
        .transition_directly = true,
      });
    } else {
      const VkImageUsageFlags usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      const auto name = std::format(
        "{0}-DepthAttachment{1}",
        config.debug_name.empty() ? "Unnamed FB" : config.debug_name,
        attachment_index);
      auto image = Core::make_ref<Image>(ImageConfiguration{
        .width = scaled_width(),
        .height = scaled_height(),
        .sample_count = config.samples,
        .format = attachment_specification.format,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .usage = usage,
        .additional_name_data = name,
        .transition_directly = true,
      });
      attachment_images.emplace_back(image);
    }
    attachment_index++;
  }

  Core::ensure(!specification.attachments.attachments.empty(),
               "Missing attachments.");
  on_resize(size, true);
}

Framebuffer::~Framebuffer()
{
  release();
}

void
Framebuffer::release()
{
  if (framebuffer == nullptr) {
    return;
  }

  vkDestroyFramebuffer(Device::the().device(), framebuffer, nullptr);
  vkDestroyRenderPass(Device::the().device(), renderpass, nullptr);
  // Don't free the images if we don't own them
  if (config.existing_framebuffer) {
    return;
  }

  Core::u32 attachment_index = 0;
  for (auto& image : attachment_images) {
    if (config.existing_images.contains(attachment_index)) {
      continue;
    }

    const auto has_zero_layer_image_view =
      image->get_layer_image_view(0) == nullptr;
    if (image->get_layer_count() == 1 ||
        (attachment_index == 0 && !has_zero_layer_image_view)) {
      image->destroy();
    }
    attachment_index++;
  }

  if (!depth_attachment_image) {
    return;
  }

  const auto index = static_cast<Core::u32>(config.attachments.size() - 1U);
  if (!config.existing_images.contains(index)) {
    depth_attachment_image->destroy();
  }
}

void
Framebuffer::on_resize(const Core::Extent& new_size, bool force)
{
  if (!force && new_size == size) {
    return;
  }

  if (!config.no_resize) {
    size = {
      static_cast<Core::u32>(static_cast<Core::f32>(new_size.width) *
                             config.scale),
      static_cast<Core::u32>(static_cast<Core::f32>(new_size.height) *
                             config.scale),
    };
  }

  invalidate();
  for (auto& callback : resize_callbacks) {
    callback(this);
  }
}

void
Framebuffer::add_resize_callback(const std::function<void(Framebuffer*)>& func)
{
  resize_callbacks.push_back(func);
}

auto
Framebuffer::get_depth_attachment() const -> const Core::Ref<Image>&
{
  return depth_attachment_image;
}

auto
Framebuffer::invalidate() -> void
{
  release();

  Allocator allocator("Framebuffer");

  std::vector<VkAttachmentDescription> attachment_descriptions;

  std::vector<VkAttachmentReference> color_attachment_references;
  VkAttachmentReference depth_attachment_reference;

  clear_values.resize(config.attachments.size());

  if (config.existing_framebuffer) {
    attachment_images.clear();
  }

  Core::u32 attachment_index = 0;
  for (auto attachment_specification : config.attachments) {
    if (Utils::is_depth_format(attachment_specification.format)) {
      if (config.existing_image) {
        depth_attachment_image = config.existing_image;
      } else if (config.existing_framebuffer) {
        depth_attachment_image =
          config.existing_framebuffer->get_depth_attachment();
      } else if (config.existing_images.contains(attachment_index)) {
        const auto& existing_image =
          config.existing_images.at(attachment_index);
        depth_attachment_image = existing_image;
      } else {
        depth_attachment_image =
          create_depth_attachment_image(attachment_specification.format);
      }

      VkAttachmentDescription& attachment_description =
        attachment_descriptions.emplace_back();
      attachment_description.flags = 0;
      attachment_description.format = attachment_specification.format;
      attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
      attachment_description.loadOp = config.clear_depth_on_load
                                        ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                        : VK_ATTACHMENT_LOAD_OP_LOAD;
      attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment_description.initialLayout =
        config.clear_depth_on_load
          ? VK_IMAGE_LAYOUT_UNDEFINED
          : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

      attachment_description.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachment_description.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      depth_attachment_reference = {
        attachment_index, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
      };

      clear_values[attachment_index].depthStencil = {
        .depth = config.depth_clear_value,
        .stencil = 0,
      };
    } else {
      Core::Ref<Image> colour_attachment;
      if (config.existing_framebuffer) {
        const auto& existing_image =
          config.existing_framebuffer->get_colour_attachment(attachment_index);
        colour_attachment = attachment_images.emplace_back(existing_image);
      } else if (config.existing_images.contains(attachment_index)) {
        const auto& existing_image = config.existing_images[attachment_index];
        colour_attachment = existing_image;
        attachment_images[attachment_index] = existing_image;
      } else {
        auto& image = attachment_images[attachment_index];
        auto& spec = image->configuration;
        spec.width = scaled_width();
        spec.height = scaled_height();
        colour_attachment = image;
        if (colour_attachment->get_layer_count() == 1) {
          colour_attachment->invalidate();
        } else if (attachment_index == 0 &&
                   config.existing_image_layers[0] == 0) {
          colour_attachment->invalidate();
          colour_attachment->create_specific_layer_image_views(
            std::span{ config.existing_image_layers });
        } else if (attachment_index == 0) {
          colour_attachment->create_specific_layer_image_views(
            std::span{ config.existing_image_layers });
        }
      }

      VkAttachmentDescription& attachment_description =
        attachment_descriptions.emplace_back();
      attachment_description.flags = 0;
      attachment_description.format = attachment_specification.format;
      attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
      attachment_description.loadOp = config.clear_colour_on_load
                                        ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                        : VK_ATTACHMENT_LOAD_OP_LOAD;
      attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment_description.initialLayout =
        config.clear_colour_on_load ? VK_IMAGE_LAYOUT_UNDEFINED
                                    : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachment_description.finalLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      const auto& clear_color = config.clear_colour;
      clear_values[attachment_index].color = { .float32 = {
                                                 clear_color.r,
                                                 clear_color.g,
                                                 clear_color.b,
                                                 clear_color.a,
                                               }, };
      color_attachment_references.emplace_back(VkAttachmentReference{
        attachment_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    }

    attachment_index++;
  }

  VkSubpassDescription subpass_description = {};
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.colorAttachmentCount =
    static_cast<Core::u32>(color_attachment_references.size());
  subpass_description.pColorAttachments = color_attachment_references.data();
  if (depth_attachment_image) {
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
  }

  // Use subpass dependencies for layout transitions
  std::vector<VkSubpassDependency> dependencies;

  if (!attachment_images.empty()) {
    {
      auto& dependency = dependencies.emplace_back();
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }
    {
      auto& dependency = dependencies.emplace_back();
      dependency.srcSubpass = 0;
      dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }
  }

  if (depth_attachment_image) {
    {
      auto& dependency = dependencies.emplace_back();
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }

    {
      auto& dependency = dependencies.emplace_back();
      dependency.srcSubpass = 0;
      dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
      dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }
  }

  // Create the actual renderpass
  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount =
    static_cast<Core::u32>(attachment_descriptions.size());
  render_pass_info.pAttachments = attachment_descriptions.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass_description;
  render_pass_info.dependencyCount =
    static_cast<Core::u32>(dependencies.size());
  render_pass_info.pDependencies = dependencies.data();

  VK_CHECK(vkCreateRenderPass(
    Device::the().device(), &render_pass_info, nullptr, &renderpass));
  // VKUtils::SetDebugUtilsObjectName(
  //   device, VK_OBJECT_TYPE_RENDER_PASS, config.debug_name, renderpass);

  std::vector<VkImageView> attachments(attachment_images.size());
  for (Core::u32 i = 0; i < attachment_images.size(); i++) {
    const auto& image = attachment_images.at(i);
    if (image->get_layer_count() > 1) {
      attachments[i] =
        image->get_layer_image_view(config.existing_image_layers.at(i));
    } else {
      attachments[i] = image->view;
    }
    Core::ensure(attachments[i] != nullptr, "Colour attachment not attached");
  }

  if (depth_attachment_image) {
    if (config.existing_image) {
      attachments.emplace_back(depth_attachment_image->get_layer_image_view(
        config.existing_image_layers[0]));
    } else {
      attachments.emplace_back(depth_attachment_image->view);
    }

    Core::ensure(attachments.back() != nullptr, "Missing last image");
  }

  VkFramebufferCreateInfo framebuffer_create_info = {};
  framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_create_info.renderPass = renderpass;
  framebuffer_create_info.attachmentCount =
    static_cast<Core::u32>(attachments.size());
  framebuffer_create_info.pAttachments = attachments.data();
  framebuffer_create_info.width = size.width;
  framebuffer_create_info.height = size.height;
  framebuffer_create_info.layers = 1;

  VK_CHECK(vkCreateFramebuffer(
    Device::the().device(), &framebuffer_create_info, nullptr, &framebuffer));
  // VKUtils::SetDebugUtilsObjectName(
  //   device, VK_OBJECT_TYPE_FRAMEBUFFER, config.debug_name, framebuffer);
}

auto
Framebuffer::create_depth_attachment_image(VkFormat format) -> Core::Ref<Image>
{
  const VkImageUsageFlags usage =
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  const auto name =
    std::format("{0}-DepthImage",
                config.debug_name.empty() ? "Unnamed FB" : config.debug_name);
  auto image = Core::make_ref<Image>(ImageConfiguration{
    .width = scaled_width(),
    .height = scaled_height(),
    .sample_count = config.samples,
    .format = format,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    .usage = usage,
    .additional_name_data = name,
    .transition_directly = true,
  });

  return image;
}

auto
Framebuffer::construct_blend_states() const
  -> std::vector<VkPipelineColorBlendAttachmentState>
{
  auto color_attachment_count = attachment_images.size();
  std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states(
    color_attachment_count);
  for (size_t i = 0; i < color_attachment_count; i++) {
    if (!config.blend) {
      break;
    }

    blend_attachment_states[i].colorWriteMask = 0xf;
    if (!config.blend) {
      break;
    }

    const auto& attachment_specification = config.attachments[i];
    FramebufferBlendMode blend_mode =
      config.blend_mode == FramebufferBlendMode::None
        ? attachment_specification.blend_mode
        : config.blend_mode;

    blend_attachment_states[i].blendEnable =
      attachment_specification.blend ? VK_TRUE : VK_FALSE;

    blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    switch (blend_mode) {
      case FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha:
        blend_attachment_states[i].srcColorBlendFactor =
          VK_BLEND_FACTOR_SRC_ALPHA;
        blend_attachment_states[i].dstColorBlendFactor =
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attachment_states[i].srcAlphaBlendFactor =
          VK_BLEND_FACTOR_SRC_ALPHA;
        blend_attachment_states[i].dstAlphaBlendFactor =
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
      case FramebufferBlendMode::OneZero:
        blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        break;
      case FramebufferBlendMode::Zero_SrcColor:
        blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attachment_states[i].dstColorBlendFactor =
          VK_BLEND_FACTOR_SRC_COLOR;
        break;
      default:
        Core::ensure(false, "Never here");
    }
  }

  return blend_attachment_states;
}

} // namespace Engine::Graphics::V2
