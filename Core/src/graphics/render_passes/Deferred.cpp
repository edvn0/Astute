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
DeferredRenderPass::create_noise_map() -> void
{
  noise_map = TextureGenerator::simplex_noise(100, 100);
}

auto
DeferredRenderPass::create_framebuffer() -> void
{
  const auto& ext = get_renderer().get_size();
  auto&& [deferred_framebuffer, _, __, ___] = get_data();
  deferred_framebuffer = Core::make_scope<Framebuffer>(FramebufferSpecification{
        .width = ext.width,
        .height = ext.height,
        .clear_colour_on_load = false,
        .attachments = {
            { .format = VK_FORMAT_R32G32B32A32_SFLOAT },
            { .format = VK_FORMAT_R32_UINT, .blend = false },
        },
        .debug_name = "Deferred",
    });
}

auto
DeferredRenderPass::create_deferred_pass() -> void
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  deferred_shader = Shader::compile_graphics_scoped(
    Core::shaders_file("deferred.vert"), Core::shaders_file("deferred.frag"));

  deferred_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = deferred_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_GREATER_OR_EQUAL,
      .override_vertex_attributes = { {} },
      .override_instance_attributes = { {} },
    });

  deferred_material = Core::make_scope<Material>(Material::Configuration{
    .shader = deferred_shader.get(),
  });
}

auto
DeferredRenderPass::create_cubemap_pass() -> void
{
  auto&& [deferred_framebuffer, _, __, ___] = get_data();

  cubemap_shader = Shader::compile_graphics_scoped(
    Core::shaders_file("cubemap.vert"), Core::shaders_file("cubemap.frag"));

  cubemap_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = cubemap_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_LESS,
      .override_vertex_attributes = { {} },
      .override_instance_attributes = { {} },
      .test_depth = false,
      .write_depth = false,
    });

  cubemap_material = Core::make_scope<Material>(Material::Configuration{
    .shader = cubemap_shader.get(),
  });
}

auto
DeferredRenderPass::set_material_uniforms() -> void
{
  auto&& [_, __, ___, deferred_material] = get_data();
  auto& input_render_pass = get_renderer().get_render_pass("MainGeometry");

  bool could = true;
  could &= deferred_material->set("position_map",
                                  input_render_pass.get_colour_attachment(0));
  could &= deferred_material->set("normal_map",
                                  input_render_pass.get_colour_attachment(1));
  could &= deferred_material->set("albedo_specular_map",
                                  input_render_pass.get_colour_attachment(2));
  could &= deferred_material->set("shadow_position_map",
                                  input_render_pass.get_colour_attachment(3));
  could &= deferred_material->set("noise_map", noise_map);
  could &= cubemap_material->set("cubemap", cubemap);

  assert(could &&
         "Could set all (including cubemap for cubemap pass) uniforms.");
}

auto
DeferredRenderPass::setup_file_watchers() -> void
{
  setup_file_watcher(Core::shaders_file("cubemap.frag").string(), true);
  setup_file_watcher(Core::shaders_file("deferred.frag").string());
}

auto
DeferredRenderPass::construct_impl() -> void
{
  create_noise_map();
  create_framebuffer();
  create_deferred_pass();
  create_cubemap_pass();
  set_material_uniforms();
  setup_file_watchers();
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
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();
  ASTUTE_PROFILE_FUNCTION();
  RendererExtensions::explicitly_clear_framebuffer(command_buffer,
                                                   *deferred_framebuffer);

  {
    ASTUTE_PROFILE_SCOPE("Cubemap");

    RendererExtensions::bind_pipeline(command_buffer, *cubemap_pipeline);
    auto* cubemap_renderer_desc_set =
      get_renderer().generate_and_update_descriptor_write_sets(
        *cubemap_material);
    auto* cubemap_material_set =
      cubemap_material->generate_and_update_descriptor_write_sets();

    std::array cubemap_desc_sets{ cubemap_renderer_desc_set,
                                  cubemap_material_set };
    RendererExtensions::bind_descriptor_sets(
      command_buffer, *cubemap_pipeline, std::span{ cubemap_desc_sets });

    vkCmdDraw(command_buffer.get_command_buffer(), 36, 1, 0, 0);
  }

  {
    ASTUTE_PROFILE_SCOPE("Deferred");
    RendererExtensions::bind_pipeline(command_buffer, *deferred_pipeline);

    auto* renderer_desc_set =
      get_renderer().generate_and_update_descriptor_write_sets(
        *deferred_material);

    auto* material_set =
      deferred_material->generate_and_update_descriptor_write_sets();

    std::array desc_sets{ renderer_desc_set, material_set };
    RendererExtensions::bind_descriptor_sets(
      command_buffer, *deferred_pipeline, std::span{ desc_sets });

    vkCmdDraw(command_buffer.get_command_buffer(), 3, 1, 0, 0);
  }
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
  cubemap_pipeline->on_resize(ext);
}

void
DeferredRenderPass::setup_file_watcher(const std::string& shader_path,
                                       const bool is_cubemap)
{
  watch->watch = Core::make_scope<filewatch::FileWatch<std::string>>(
    shader_path, [=, this](const auto& path, filewatch::Event change_type) {
      handle_file_change(path, change_type, is_cubemap);
    });
}

void
DeferredRenderPass::handle_file_change(const std::string& path,
                                       filewatch::Event change_type,
                                       const bool is_cubemap)
{
  Core::Application::submit_post_frame_function([=, this]() {
    log_shader_change(path, change_type);
    if (change_type == filewatch::Event::modified) {
      reload_shader(is_cubemap);
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
DeferredRenderPass::reload_shader(const bool is_cubemap)
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  if (!is_cubemap) {

    if (auto maybe_shader =
          Shader::compile_graphics_scoped(Core::shaders_file("deferred.vert"),
                                          Core::shaders_file("deferred.frag"),
                                          true)) {
      std::unique_lock<std::mutex> lock(RenderPass::get_mutex());
      deferred_shader = std::move(maybe_shader);
      recreate_pipeline();
    }
  } else {
    if (auto maybe_shader =
          Shader::compile_graphics_scoped(Core::shaders_file("cubemap.vert"),
                                          Core::shaders_file("cubemap.frag"),
                                          true)) {
      std::unique_lock<std::mutex> lock(RenderPass::get_mutex());
      cubemap_shader = std::move(maybe_shader);
      recreate_pipeline(true);
    }
  }
}

void
DeferredRenderPass::recreate_pipeline(const bool is_cubemap)
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();

  if (is_cubemap) {
    cubemap_pipeline =
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer = deferred_framebuffer.get(),
        .shader = cubemap_shader.get(),
        .sample_count = VK_SAMPLE_COUNT_1_BIT,
        .depth_comparator = VK_COMPARE_OP_LESS,
        .override_vertex_attributes = {},
        .override_instance_attributes = {},
      });
  } else {
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
}

} // namespace Engine::Graphics
