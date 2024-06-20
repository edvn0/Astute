#include "pch/CorePCH.hpp"

#include "graphics/RenderPass.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

RenderPass::RenderPass(Renderer& input)
  : renderer(input)
{
}

auto
RenderPass::construct() -> void
{
  construct_impl();

  is_compute = false;
  if (const auto& pipe = std::get<Core::Scope<IPipeline>>(pass);
      pipe != nullptr) {
    is_compute = std::get<Core::Scope<IPipeline>>(pass)->get_bind_point() ==
                 VK_PIPELINE_BIND_POINT_COMPUTE;
  }
}

auto
RenderPass::execute(CommandBuffer& command_buffer) -> void
{
#ifdef ASTUTE_DEBUG
  if (!is_valid()) {
    return;
  }
#endif
  auto performance_struct =
    Renderer::create_gpu_performance_scope(command_buffer, name());
  bind(command_buffer);
  execute_impl(command_buffer);
  unbind(command_buffer);
}

auto
RenderPass::get_colour_attachment(Core::u32 index) const
  -> const Core::Ref<Image>&
{
  return get_framebuffer()->get_colour_attachment(index);
}

auto
RenderPass::get_depth_attachment() const -> const Core::Ref<Image>&
{
  if (const auto& framebuffer = get_framebuffer();
      framebuffer != nullptr && framebuffer->has_depth_attachment()) {
    return framebuffer->get_depth_attachment();
  }

  const auto& extraneous_framebuffer = get_extraneous_framebuffer(0);
  return extraneous_framebuffer->get_depth_attachment();
}

auto
RenderPass::bind(CommandBuffer& command_buffer) -> void
{
  const auto& pipeline = std::get<Core::Scope<IPipeline>>(pass);
  if (!is_compute) {
    RendererExtensions::begin_renderpass(command_buffer, *get_framebuffer());
  }

  vkCmdBindPipeline(command_buffer.get_command_buffer(),
                    pipeline->get_bind_point(),
                    pipeline->get_pipeline());
}

auto
RenderPass::unbind(CommandBuffer& command_buffer) -> void
{
  if (!is_compute) {
    RendererExtensions::end_renderpass(command_buffer);
  }
}

auto
RenderPass::generate_and_update_descriptor_write_sets(Material& material)
  -> VkDescriptorSet
{
  return renderer.generate_and_update_descriptor_write_sets(material);
}

auto
RenderPass::blit_to(const CommandBuffer&, const Framebuffer&, BlitProperties)
  -> void
{
}

} // namespace Engine::Graphic
