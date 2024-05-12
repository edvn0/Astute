#pragma once

#include <vulkan/vulkan.h>

#include "core/Types.hpp"
#include "graphics/IFramebuffer.hpp"

#include "graphics/Forward.hpp"

#include <glm/glm.hpp>
#include <unordered_map>

namespace Engine::Graphics {

class Framebuffer;

enum class FramebufferBlendMode
{
  None = 0,
  OneZero,
  SrcAlphaOneMinusSrcAlpha,
  Additive,
  Zero_SrcColor
};

struct FramebufferTextureSpecification
{
  FramebufferTextureSpecification() = default;
  explicit(false) FramebufferTextureSpecification(VkFormat fmt)
    : format(fmt)
  {
  }

  VkFormat format;
  bool blend{ true };
  FramebufferBlendMode blend_mode{
    FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha
  };
};

struct FramebufferAttachmentSpecification
{
  FramebufferAttachmentSpecification() = default;
  FramebufferAttachmentSpecification(
    const std::initializer_list<FramebufferTextureSpecification>& input)
    : attachments(input)
  {
  }

  std::vector<FramebufferTextureSpecification> attachments;

  auto begin() -> auto { return std::begin(attachments); }
  auto end() -> auto { return std::end(attachments); }
  auto begin() const -> auto { return std::cbegin(attachments); }
  auto end() const -> auto { return std::cend(attachments); }

  auto size() const { return attachments.size(); };

  auto operator[](auto i) -> decltype(auto) { return attachments.at(i); }
  auto operator[](auto i) const -> decltype(auto) { return attachments.at(i); }
};

struct FramebufferSpecification
{
  float scale = 1.0f;
  Core::u32 width = 0;
  Core::u32 height = 0;
  glm::vec4 clear_colour = { 0.0f, 0.0f, 0.0f, 1.0f };
  float depth_clear_value = 0.0f;
  bool clear_colour_on_load = true;
  bool clear_depth_on_load = true;

  FramebufferAttachmentSpecification attachments;
  VkSampleCountFlagBits samples{ VK_SAMPLE_COUNT_1_BIT };

  bool no_resize = false;

  bool blend = true;
  FramebufferBlendMode blend_mode = FramebufferBlendMode::None;

  bool transfer = false;

  Core::Ref<Image> existing_image{ nullptr };
  std::vector<Core::u32> existing_image_layers{};

  std::unordered_map<Core::u32, Core::Ref<Image>> existing_images{};

  Core::Ref<Framebuffer> existing_framebuffer{ nullptr };
  std::string debug_name{ "Framebuffer" };
};

class Framebuffer : public IFramebuffer
{
public:
  explicit Framebuffer(const FramebufferSpecification&);
  ~Framebuffer();
  auto on_resize(const Core::Extent&, bool force) -> void;
  auto on_resize(const Core::Extent& ext) -> void override
  {
    on_resize(ext, true);
  }
  auto add_resize_callback(const std::function<void(Framebuffer*)>&) -> void;

  auto get_name() const -> const std::string& override
  {
    return config.debug_name;
  }
  auto get_renderpass() -> VkRenderPass override { return renderpass; }
  auto get_renderpass() const -> VkRenderPass override { return renderpass; }
  auto get_framebuffer() -> VkFramebuffer override { return framebuffer; }
  auto get_framebuffer() const -> VkFramebuffer override { return framebuffer; }
  auto get_extent() -> VkExtent2D override
  {
    return {
      size.width,
      size.height,
    };
  }
  auto get_extent() const -> VkExtent2D override
  {
    return {
      size.width,
      size.height,
    };
  }
  auto scaled_width() const
  {
    return static_cast<Core::u32>(size.width * config.scale);
  }
  auto scaled_height() const
  {
    return static_cast<Core::u32>(size.height * config.scale);
  }

  auto get_colour_attachment(Core::u32 index = 0) const
    -> const Core::Ref<Image>& override
  {
    return attachment_images.at(index);
  }
  auto get_colour_attachment_count() const -> Core::u32 override
  {
    return static_cast<Core::u32>(attachment_images.size());
  }
  auto get_depth_attachment() const -> const Core::Ref<Image>& override;
  auto invalidate() -> void override;
  auto release() -> void override;

  auto get_clear_values() const -> const std::vector<VkClearValue>& override
  {
    return clear_values;
  }
  auto has_depth_attachment() const -> bool override
  {
    return depth_attachment_image != nullptr;
  }
  auto construct_blend_states() const
    -> std::vector<VkPipelineColorBlendAttachmentState> override;

  auto get_specification() const -> const FramebufferSpecification&;

private:
  FramebufferSpecification config;
  Core::Extent size;

  std::vector<Core::Ref<Image>> attachment_images;
  Core::Ref<Image> depth_attachment_image;

  auto create_depth_attachment_image(VkFormat) -> Core::Ref<Image>;

  std::vector<VkClearValue> clear_values;

  VkRenderPass renderpass{ nullptr };
  VkFramebuffer framebuffer{ nullptr };

  std::vector<std::function<void(Framebuffer*)>> resize_callbacks;
};

} // namespace Engine::Core
