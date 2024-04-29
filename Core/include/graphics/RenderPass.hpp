#pragma once

#include "graphics/Forward.hpp"

#include "core/FrameBasedCollection.hpp"
#include "core/Types.hpp"

#include <tuple>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class RenderPass
{
public:
  virtual ~RenderPass() = default;

  virtual auto construct() -> void = 0;
  virtual auto on_resize(const Core::Extent& new_size) -> void = 0;
  auto execute(CommandBuffer& command_buffer) -> void
  {
    bind(command_buffer);
    execute_impl(command_buffer);
    unbind(command_buffer);
  }

  auto destruct() -> void
  {
    destruct_impl();

    pass.clear();
  }

  auto get_colour_attachment(Core::u32) const -> const Core::Ref<Image>&;
  auto get_depth_attachment() const -> const Core::Ref<Image>&;
  auto get_framebuffer() -> decltype(auto)
  {
    return std::get<Core::Scope<Framebuffer>>(*pass);
  }
  auto get_framebuffer() const -> const auto&
  {
    return std::get<Core::Scope<Framebuffer>>(*pass);
  }

  struct BlitProperties
  {
    std::optional<Core::u32> colour_attachment_index{};
    bool depth_attachment{ false };
  };
  auto blit_to(const CommandBuffer&, const Framebuffer&, BlitProperties = {})
    -> void;

protected:
  explicit RenderPass(Renderer& input)
    : renderer(input)
  {
  }
  virtual auto destruct_impl() -> void = 0;
  virtual auto execute_impl(CommandBuffer&) -> void = 0;
  virtual auto bind(CommandBuffer& command_buffer) -> void;
  virtual auto unbind(CommandBuffer& command_buffer) -> void;
  auto generate_and_update_descriptor_write_sets(Material&) -> VkDescriptorSet;
  auto get_data() -> auto& { return *pass; }
  auto get_renderer() -> Renderer& { return renderer; }

  auto for_each(auto&& func) { pass.for_each(func); }

private:
  Renderer& renderer;
  using RenderTuple = std::tuple<Core::Scope<Framebuffer>,
                                 Core::Scope<Shader>,
                                 Core::Scope<GraphicsPipeline>,
                                 Core::Scope<Material>>;
  Core::FrameBasedCollection<RenderTuple> pass{};

  friend class Renderer;
};

} // namespace Engine::Graphics
