#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Deferred.hpp"

#include "core/Scene.hpp"
#include "logging/Logger.hpp"

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
#include "graphics/TextureGenerator.hpp"
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

struct Impl
{
  Core::Scope<filewatch::FileWatch<std::string>> watch;
};

auto
DeferredRenderPass::construct_impl() -> void
{
  noise_map = TextureGenerator::simplex_noise(100, 100);
  const auto& ext = get_renderer().get_size();
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();
  deferred_framebuffer = Core::make_scope<Framebuffer>(FramebufferSpecification{
    .width = ext.width,
    .height = ext.height,
    .attachments = {
      {
                       .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                     },

                     {
                       .format = VK_FORMAT_R32_UINT,
                       .blend = false,
                     }, },
    .debug_name = "Deferred",
  });

  deferred_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/deferred.vert", "Assets/shaders/deferred.frag");
  deferred_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = deferred_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
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
  deferred_material->set("cubemap", cubemap);
  deferred_material->set("position_map",
                         input_render_pass.get_colour_attachment(0));
  deferred_material->set("normal_map",
                         input_render_pass.get_colour_attachment(1));
  deferred_material->set("albedo_specular_map",
                         input_render_pass.get_colour_attachment(2));
  deferred_material->set("shadow_position_map",
                         input_render_pass.get_colour_attachment(3));
  deferred_material->set("noise_map", noise_map);

  setup_file_watcher("Assets/shaders/deferred.frag");
}

DeferredRenderPass::DeferredRenderPass(Renderer& ren,
                                       const Core::Ref<Image>& cube)
  : RenderPass(ren)
  , cubemap(cube)
{
  watch = Core::make_scope<Impl>();
}

DeferredRenderPass::~DeferredRenderPass()
{
  watch.reset();
}

auto
DeferredRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();

  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  auto* renderer_desc_set =
    get_renderer().generate_and_update_descriptor_write_sets(
      *deferred_material);

  auto* material_set =
    deferred_material->generate_and_update_descriptor_write_sets();

  std::array desc_sets{ renderer_desc_set, material_set };
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          deferred_pipeline->get_bind_point(),
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
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);
}

void
DeferredRenderPass::setup_file_watcher(const std::string& shader_path)
{
  watch->watch = Core::make_scope<filewatch::FileWatch<std::string>>(
    shader_path, [this](const auto& path, filewatch::Event change_type) {
      handle_file_change(path, change_type);
    });
}

void
DeferredRenderPass::handle_file_change(const std::string& path,
                                       filewatch::Event change_type)
{
  Core::Application::submit_post_frame_function([this, path, change_type]() {
    log_shader_change(path, change_type);
    if (change_type == filewatch::Event::modified) {
      reload_shader();
    }
  });
}

void
DeferredRenderPass::log_shader_change(const std::string& path,
                                      filewatch::Event change_type)
{
  info("Shader path {} had an event of type: '{}'", path, change_type);
}

void
DeferredRenderPass::reload_shader()
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  if (auto maybe_shader = Shader::compile_graphics_scoped(
        "Assets/shaders/deferred.vert", "Assets/shaders/deferred.frag", true)) {
    std::unique_lock<std::mutex> lock(RenderPass::get_mutex());
    deferred_shader = std::move(maybe_shader);
    recreate_pipeline();
  }
}

void
DeferredRenderPass::recreate_pipeline()
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  deferred_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = deferred_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_LESS,
      .override_vertex_attributes = {},
      .override_instance_attributes = {},
    });
}

} // namespace Engine::Graphics
