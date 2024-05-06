#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Deferred.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "core/Application.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Image.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

#include <FileWatch.hpp>

template<>
struct std::formatter<filewatch::Event>
  : public std::formatter<std::string_view>
{
  auto format(const filewatch::Event& type, std::format_context& ctx) const
  {
    return std::format_to(ctx.out(), "{}", to_string(type));
  }

private:
  static constexpr auto to_string(auto type) -> std::string_view
  {
    switch (type) {
      case filewatch::Event::added:
        return "Added";
      case filewatch::Event::removed:
        return "Removed";
      case filewatch::Event::modified:
        return "Modified";
      case filewatch::Event::renamed_old:
        return "RenamedOld";
      case filewatch::Event::renamed_new:
        return "RenamedNew";
    }
    return "Missing";
  }
};

namespace Engine::Graphics {

auto
DeferredRenderPass::construct() -> void
{
  on_resize(get_renderer().get_size());

  watch = Core::make_scope<filewatch::FileWatch<std::string>>(
    "Assets/shaders/deferred.frag",
    [this](const auto& path, const filewatch::Event change_type) {
      Device::the().wait();
      info("Path {} had an event of type: '{}'", path, change_type);
      auto&& [deferred_framebuffer,
              deferred_shader,
              deferred_pipeline,
              deferred_material] = get_data();

      auto maybe_deferred_shader = Shader::compile_graphics_scoped(
        "Assets/shaders/deferred.vert", "Assets/shaders/deferred.frag", true);
      if (!maybe_deferred_shader)
        return;

      std::unique_lock lock{ RenderPass::get_mutex() };

      deferred_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = deferred_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .cull_mode = VK_CULL_MODE_BACK_BIT,
      .depth_comparator = VK_COMPARE_OP_LESS,
      .override_vertex_attributes = {
          {  },
        },
      .override_instance_attributes = {
          {  },
        },
    });
    });
}

auto
DeferredRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  auto renderer_desc_set =
    get_renderer().generate_and_update_descriptor_write_sets(
      *deferred_material);

  auto material_set =
    deferred_material->generate_and_update_descriptor_write_sets();

  std::array desc_sets{ renderer_desc_set, material_set };
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          deferred_pipeline->get_layout(),
                          0,
                          static_cast<Core::u32>(desc_sets.size()),
                          desc_sets.data(),
                          0,
                          nullptr);

  vkCmdDraw(command_buffer.get_command_buffer(), 3, 1, 0, 0);
}

auto
DeferredRenderPass::destruct_impl() -> void
{
  watch.reset();
}

auto
DeferredRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();
  deferred_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = get_renderer().get_size(),
      .colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT, },
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .immediate_construction = false,
      .name = "Deferred",
    });
  deferred_framebuffer->add_resolve_for_colour(0);
  deferred_framebuffer->create_framebuffer_fully();

  deferred_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/deferred.vert", "Assets/shaders/deferred.frag");
  deferred_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = deferred_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .cull_mode = VK_CULL_MODE_BACK_BIT,
      .depth_comparator = VK_COMPARE_OP_LESS,
      .override_vertex_attributes = {
          {  },
        },
      .override_instance_attributes = {
          {  },
        },
    });

  deferred_material = Core::make_scope<Material>(Material::Configuration{
    .shader = deferred_shader.get(),
  });

  auto& input_render_pass = get_renderer().get_render_pass("MainGeometry");
  deferred_material->set("position_map",
                         input_render_pass.get_colour_attachment(0));
  deferred_material->set("normal_map",
                         input_render_pass.get_colour_attachment(1));
  deferred_material->set("albedo_specular_map",
                         input_render_pass.get_colour_attachment(2));
  deferred_material->set("shadow_position_map",
                         input_render_pass.get_colour_attachment(3));
  deferred_material->set(
    "shadow_map",
    get_renderer().get_render_pass("Shadow").get_depth_attachment());
}

} // namespace Engine::Graphics
