#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Image.hpp"

#include "core/Verify.hpp"

namespace Engine::Graphics {

namespace Utils {
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
}

Framebuffer::Framebuffer(const FramebufferSpecification& specification)
  : config(specification)
{
  size = {
    static_cast<Core::u32>(config.width * config.scale),
    static_cast<Core::u32>(config.height * config.scale),
  };

  if (!size.valid())
    throw std::runtime_error("What!!!");

  if (config.existing_framebuffer)
    return;

  Core::u32 attachment_index = 0;
  for (auto& attachment_specification : config.attachments) {
    if (config.existing_image && config.existing_image->get_layer_count() > 1) {
      if (Utils::is_depth_format(attachment_specification.format))
        depth_attachment_image = config.existing_image;
      else
        attachment_images.emplace_back(config.existing_image);
    } else if (config.existing_images.contains(attachment_index)) {
      if (!Utils::is_depth_format(attachment_specification.format))
        attachment_images.emplace_back(); // This will be set later
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
      });
      attachment_images.emplace_back(image);
    }
    attachment_index++;
  }

  Core::ensure(specification.attachments.attachments.size(),
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
  if (!framebuffer)
    return;

  vkDestroyFramebuffer(Device::the().device(), framebuffer, nullptr);
  vkDestroyRenderPass(Device::the().device(), renderpass, nullptr);
  // Don't free the images if we don't own them
  if (config.existing_framebuffer)
    return;

  Core::u32 attachment_index = 0;
  for (auto image : attachment_images) {
    if (config.existing_images.contains(attachment_index)) {
      continue;
    }

    if (image->get_layer_count() == 1 ||
        (attachment_index == 0 && !image->get_layer_image_view(0))) {
      image->destroy();
    }
    attachment_index++;
  }

  if (!depth_attachment_image)
    return;

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
      static_cast<Core::u32>(new_size.width * config.scale),
      static_cast<Core::u32>(new_size.height * config.scale),
    };
  }

  invalidate();
  for (auto& callback : resize_callbacks)
    callback(this);
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

  std::vector<VkAttachmentDescription> attachmentDescriptions;

  std::vector<VkAttachmentReference> colorAttachmentReferences;
  VkAttachmentReference depthAttachmentReference;

  clear_values.resize(config.attachments.size());

  if (config.existing_framebuffer)
    attachment_images.clear();

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

      VkAttachmentDescription& attachmentDescription =
        attachmentDescriptions.emplace_back();
      attachmentDescription.flags = 0;
      attachmentDescription.format = attachment_specification.format;
      attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescription.loadOp = config.clear_depth_on_load
                                       ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                       : VK_ATTACHMENT_LOAD_OP_LOAD;
      attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription.initialLayout =
        config.clear_depth_on_load
          ? VK_IMAGE_LAYOUT_UNDEFINED
          : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

      attachmentDescription.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachmentDescription.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      depthAttachmentReference = {
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
        // ImageSpecification& spec = image->GetSpecification();
        // spec.width = (Core::u32)(m_Width * config.Scale);
        // spec.Height = (Core::u32)(m_Height * config.Scale);
        colour_attachment = image;
        if (colour_attachment->get_layer_count() == 1)
          colour_attachment->invalidate();
        else if (attachment_index == 0 &&
                 config.existing_image_layers[0] == 0) {
          colour_attachment->invalidate();
          colour_attachment->create_specific_layer_image_views(
            std::span{ config.existing_image_layers });
        } else if (attachment_index == 0) {
          colour_attachment->create_specific_layer_image_views(
            std::span{ config.existing_image_layers });
        }
      }

      VkAttachmentDescription& attachmentDescription =
        attachmentDescriptions.emplace_back();
      attachmentDescription.flags = 0;
      attachmentDescription.format = attachment_specification.format;
      attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescription.loadOp = config.clear_colour_on_load
                                       ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                       : VK_ATTACHMENT_LOAD_OP_LOAD;
      attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription.initialLayout =
        config.clear_colour_on_load ? VK_IMAGE_LAYOUT_UNDEFINED
                                    : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachmentDescription.finalLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      const auto& clearColor = config.clear_colour;
      clear_values[attachment_index].color = {
        .float32 = { clearColor.r, clearColor.g, clearColor.b, clearColor.a }
      };
      colorAttachmentReferences.emplace_back(VkAttachmentReference{
        attachment_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    }

    attachment_index++;
  }

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount =
    Core::u32(colorAttachmentReferences.size());
  subpassDescription.pColorAttachments = colorAttachmentReferences.data();
  if (depth_attachment_image)
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

  // TODO: do we need these?
  // Use subpass dependencies for layout transitions
  std::vector<VkSubpassDependency> dependencies;

  if (attachment_images.size()) {
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
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount =
    static_cast<Core::u32>(attachmentDescriptions.size());
  renderPassInfo.pAttachments = attachmentDescriptions.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<Core::u32>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK(vkCreateRenderPass(
    Device::the().device(), &renderPassInfo, nullptr, &renderpass));
  // VKUtils::SetDebugUtilsObjectName(
  //   device, VK_OBJECT_TYPE_RENDER_PASS, config.debug_name, renderpass);

  std::vector<VkImageView> attachments(attachment_images.size());
  for (Core::u32 i = 0; i < attachment_images.size(); i++) {
    const auto& image = attachment_images.at(i);
    if (image->get_layer_count() > 1)
      attachments[i] =
        image->get_layer_image_view(config.existing_image_layers.at(i));
    else {
      attachments[i] = image->view;
    }
    Core::ensure(attachments[i], "Colour attachment not attached");
  }

  if (depth_attachment_image) {
    if (config.existing_image) {
      attachments.emplace_back(depth_attachment_image->get_layer_image_view(
        config.existing_image_layers[0]));
    } else {
      attachments.emplace_back(depth_attachment_image->view);
    }

    Core::ensure(attachments.back(), "Missing last image");
  }

  VkFramebufferCreateInfo framebufferCreateInfo = {};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCreateInfo.renderPass = renderpass;
  framebufferCreateInfo.attachmentCount = Core::u32(attachments.size());
  framebufferCreateInfo.pAttachments = attachments.data();
  framebufferCreateInfo.width = size.width;
  framebufferCreateInfo.height = size.height;
  framebufferCreateInfo.layers = 1;

  VK_CHECK(vkCreateFramebuffer(
    Device::the().device(), &framebufferCreateInfo, nullptr, &framebuffer));
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
  });

  return image;
}

auto
Framebuffer::construct_blend_states() const
  -> std::vector<VkPipelineColorBlendAttachmentState>
{
  size_t colorAttachmentCount = attachment_images.size();
  std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(
    colorAttachmentCount);
  for (size_t i = 0; i < colorAttachmentCount; i++) {
    if (!config.blend)
      break;

    blendAttachmentStates[i].colorWriteMask = 0xf;
    if (!config.blend)
      break;

    const auto& attachment_specification = config.attachments[i];
    FramebufferBlendMode blendMode =
      config.blend_mode == FramebufferBlendMode::None
        ? attachment_specification.blend_mode
        : config.blend_mode;

    blendAttachmentStates[i].blendEnable =
      attachment_specification.blend ? VK_TRUE : VK_FALSE;

    blendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    switch (blendMode) {
      case FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha:
        blendAttachmentStates[i].srcColorBlendFactor =
          VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentStates[i].dstColorBlendFactor =
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentStates[i].srcAlphaBlendFactor =
          VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentStates[i].dstAlphaBlendFactor =
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
      case FramebufferBlendMode::OneZero:
        blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        break;
      case FramebufferBlendMode::Zero_SrcColor:
        blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentStates[i].dstColorBlendFactor =
          VK_BLEND_FACTOR_SRC_COLOR;
        break;
      default:
        Core::ensure(false, "Never here");
    }
  }

  return blendAttachmentStates;
}

} // namespace Engine::Graphics::V2
