#pragma once

#include "graphics/Forward.hpp"

#include "core/Types.hpp"

#include <tuple>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class RenderPass
{
public:
  virtual ~RenderPass() = default;

  virtual auto construct(Renderer&) -> void = 0;
  virtual auto on_resize(Renderer& renderer, const Core::Extent& new_size)
    -> void = 0;
  auto execute(Renderer& renderer, CommandBuffer& command_buffer) -> void
  {
    bind(command_buffer);
    execute_impl(renderer, command_buffer);
    unbind(command_buffer);
  }

  auto destruct() -> void
  {
    destruct_impl();

    if (framebuffer)
      framebuffer.reset();
    if (shader)
      shader.reset();
    if (pipeline)
      pipeline.reset();
    if (material)
      material.reset();
  }

  auto update_attachment(Core::u32, const Core::Ref<Image>&) -> void;
  auto get_colour_attachment(Core::u32) const -> const Core::Ref<Image>&;
  auto get_depth_attachment() const -> const Core::Ref<Image>&;

protected:
  virtual auto destruct_impl() -> void = 0;
  virtual auto execute_impl(Renderer&, CommandBuffer&) -> void = 0;
  auto generate_and_update_descriptor_write_sets(Renderer&, Material&)
    -> VkDescriptorSet;
  auto get_data() -> std::tuple<Core::Scope<Framebuffer>&,
                                Core::Scope<Shader>&,
                                Core::Scope<GraphicsPipeline>&,
                                Core::Scope<Material>&>
  {
    return std::tuple<Core::Scope<Framebuffer>&,
                      Core::Scope<Shader>&,
                      Core::Scope<GraphicsPipeline>&,
                      Core::Scope<Material>&>{
      framebuffer, shader, pipeline, material
    };
  }

private:
  Core::Scope<Framebuffer> framebuffer{ nullptr };
  Core::Scope<Shader> shader{ nullptr };
  Core::Scope<GraphicsPipeline> pipeline{ nullptr };
  Core::Scope<Material> material{ nullptr };

  auto bind(CommandBuffer& command_buffer) -> void;
  auto unbind(CommandBuffer& command_buffer) -> void;

  friend class Renderer;
};

} // namespace Engine::Graphics
