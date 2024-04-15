#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "graphics/GPUBuffer.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};
static constexpr std::array<Vertex, 4> quad_vertices = {
  Vertex{
    { -0.5F, -0.5F, 0.0F },
    { 0.0F, 0.0F, 1.0F },
    { 0.0F, 0.0F },
  },
  Vertex{
    { 0.5F, -0.5F, 0.0F },
    { 0.0F, 0.0F, 1.0F },
    { 1.0F, 0.0F },
  },
  Vertex{
    { 0.5F, 0.5F, 0.0F },
    { 0.0F, 0.0F, 1.0F },
    { 1.0F, 1.0F },
  },
  Vertex{
    { -0.5F, 0.5F, 0.0F },
    { 0.0F, 0.0F, 1.0F },
    { 0.0F, 1.0F },
  },
};
static constexpr auto quad_indices = std::array{
  0U, 1U, 2U, 2U, 3U, 0U,
};

Renderer::Renderer(Configuration config, const Window* window)
  : size(window->get_swapchain().get_size())
  , shadow_pass_parameters({ config.shadow_pass_size })
{
  vertex_buffer = Core::make_scope<VertexBuffer>(std::span{
    quad_vertices.data(),
    quad_vertices.size(),
  });
  index_buffer = Core::make_scope<IndexBuffer>(std::span{ quad_indices });
  predepth_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = window->get_swapchain().get_size(),
    });
  predepth_shader =
    Core::make_scope<Shader>("shaders/predepth.vert", "shaders/predepth.frag");
  predepth_pipeline = Core::make_scope<GraphicsPipeline>();

  info("Vertex Size: {}", vertex_buffer->size());
  info("Index Size: {}", index_buffer->size());

  command_buffer = Core::make_scope<CommandBuffer>(CommandBuffer::Properties{
    .image_count = window->get_swapchain().get_image_count(),
    .queue_type = QueueType::Graphics,
    .primary = true,
  });
}

auto
Renderer::begin_scene(Core::Scene& scene, const SceneRendererCamera& camera)
  -> void
{
}

auto
Renderer::end_scene() -> void
{
  flush_draw_lists();
}

auto
Renderer::predepth_pass() -> void
{
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = predepth_framebuffer->get_renderpass();
  render_pass_info.framebuffer = predepth_framebuffer->get_framebuffer();
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = predepth_framebuffer->get_extent();
  const auto& clear_values = predepth_framebuffer->get_clear_values();
  render_pass_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  RendererExtensions::begin_renderpass(*command_buffer, *predepth_framebuffer);

  RendererExtensions::explicitly_clear_framebuffer(*command_buffer,
                                                   *predepth_framebuffer);

  vkCmdBindPipeline(command_buffer->get_command_buffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    predepth_pipeline->get_pipeline());
  auto vertex_buffers = std::array{ vertex_buffer->get_buffer() };
  auto offsets = std::array<VkDeviceSize, 1>{ 0 };
  vkCmdBindVertexBuffers(command_buffer->get_command_buffer(),
                         0,
                         1,
                         vertex_buffers.data(),
                         offsets.data());
  vkCmdBindIndexBuffer(command_buffer->get_command_buffer(),
                       index_buffer->get_buffer(),
                       0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(command_buffer->get_command_buffer(),
                   static_cast<Core::u32>(index_buffer->size()),
                   1,
                   0,
                   0,
                   0);

  RendererExtensions::end_renderpass(*command_buffer);
}

auto
Renderer::flush_draw_lists() -> void
{
  command_buffer->begin();
  // Predepth pass
  predepth_pass();

  // Shadow pass
  // Geometry pass
  command_buffer->end();
  command_buffer->submit();
}

} // namespace Engine::Graphics
