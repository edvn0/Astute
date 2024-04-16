#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "graphics/DescriptorResource.hpp"
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
static constexpr std::array<const Vertex, 4> quad_vertices_counter_clockwise = {
  Vertex{ .position = { -1.0f, -1.0f, 0.0f },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 0.0f, 0.0f } },
  Vertex{ .position = { 1.0f, -1.0f, 0.0f },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 1.0f, 0.0f } },
  Vertex{ .position = { 1.0f, 1.0f, 0.0f },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 1.0f, 1.0f } },
  Vertex{ .position = { -1.0f, 1.0f, 0.0f },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 0.0f, 1.0f } },
};

static constexpr auto quad_indices = std::array{
  0U, 1U, 2U, 2U, 3U, 0U,
};

Renderer::Renderer(Configuration config, const Window* window)
  : size(window->get_swapchain().get_size())
  , shadow_pass_parameters({ config.shadow_pass_size })
{
  Shader::initialise_compiler(Compilation::ShaderCompilerConfiguration{
    .optimisation_level = 0,
    .debug_information_level = Compilation::DebugInformationLevel::None,
    .warnings_as_errors = false,
    .include_directories = { std::filesystem::path{ "shaders" } },
    .macro_definitions = {},
  });

  vertex_buffer = Core::make_scope<VertexBuffer>(
    std::span{ quad_vertices_counter_clockwise });
  index_buffer = Core::make_scope<IndexBuffer>(std::span{ quad_indices });
  predepth_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = window->get_swapchain().get_size(),
      .colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT,
                                     VK_FORMAT_R32G32B32A32_SFLOAT,
                                     VK_FORMAT_R32G32B32A32_SFLOAT },
      .depth_attachment_format = VK_FORMAT_D32_SFLOAT,
    });
  predepth_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/predepth.vert", "Assets/shaders/predepth.frag");
  predepth_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = predepth_framebuffer.get(),
      .shader = predepth_shader.get(),
    });

  command_buffer = Core::make_scope<CommandBuffer>(CommandBuffer::Properties{
    .image_count = window->get_swapchain().get_image_count(),
    .queue_type = QueueType::Graphics,
    .primary = true,
  });
}

Renderer::~Renderer()
{
  command_buffer.reset();
}

auto
Renderer::begin_scene(Core::Scene& scene, const SceneRendererCamera& camera)
  -> void
{
  auto& [view, proj, view_proj, camera_pos] = renderer_ubo.get_data();
  view = camera.camera.get_view_matrix();
  proj = camera.camera.get_projection_matrix();
  view_proj = proj * view;
  camera_pos = camera.camera.get_position();
  renderer_ubo.update();
}

auto
Renderer::end_scene() -> void
{
  flush_draw_lists();
}

auto
Renderer::on_resize(const Core::Extent&) -> void
{
  predepth_framebuffer->on_resize(size);
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

  // RendererExtensions::explicitly_clear_framebuffer(*command_buffer,
  //                                                 *predepth_framebuffer);

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

  auto write = predepth_shader->get_descriptor_set("RendererUBO", 0);
  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  const auto& layouts = predepth_shader->get_descriptor_set_layouts();
  alloc_info.descriptorSetCount = static_cast<Core::u32>(layouts.size());
  alloc_info.pSetLayouts = layouts.data();

  auto allocated =
    DescriptorResource::the().allocate_descriptor_set(alloc_info);

  VkWriteDescriptorSet write_descriptor_set{};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = allocated;
  write_descriptor_set.dstBinding = write->dstBinding;
  write_descriptor_set.dstArrayElement = write->dstBinding;
  write_descriptor_set.descriptorType = write->descriptorType;
  write_descriptor_set.descriptorCount = write->descriptorCount;
  write_descriptor_set.pBufferInfo = &renderer_ubo.get_descriptor_info();
  vkUpdateDescriptorSets(
    Device::the().device(), 1, &write_descriptor_set, 0, nullptr);

  vkCmdBindDescriptorSets(command_buffer->get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          predepth_pipeline->get_layout(),
                          0,
                          1,
                          &allocated,
                          0,
                          nullptr);

  vkCmdBindIndexBuffer(command_buffer->get_command_buffer(),
                       index_buffer->get_buffer(),
                       0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(command_buffer->get_command_buffer(),
                   static_cast<Core::u32>(index_buffer->count()),
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
