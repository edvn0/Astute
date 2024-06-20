#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Bloom.hpp"
#include "graphics/render_passes/Composition.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"

#include <imgui.h>

namespace Engine::Graphics {

auto
CompositionRenderPass::construct_impl() -> void
{
  const auto& ext = get_renderer().get_size();
  auto&& [composition_framebuffer,
          composition_shader,
          composition_pipeline,
          composition_material] = get_data();
  composition_framebuffer =
    Core::make_scope<Framebuffer>(FramebufferSpecification{
      .width = ext.width,
      .height = ext.height,
      .attachments = { {.format = VK_FORMAT_R8G8B8A8_SRGB}, },
      .debug_name = "Composition",
    });

  composition_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/composition.vert", "Assets/shaders/composition.frag");
  composition_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = composition_framebuffer.get(),
      .shader = composition_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_LESS,
      .override_vertex_attributes = {
          {  },
        },
      .override_instance_attributes = {
          {  },
        },
    });

  composition_material = Core::make_scope<Material>(Material::Configuration{
    .shader = composition_shader.get(),
  });
  const auto& input_render_pass =
    get_renderer().get_render_pass("ChromaticAberration");
  composition_material->set("fullscreen_texture",
                            input_render_pass.get_colour_attachment(0));
  composition_material->set(
    "bloom_texture",
    static_cast<BloomRenderPass&>(get_renderer().get_render_pass("Bloom"))
      .get_bloom_texture_output());
}

auto
CompositionRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();
  auto&& [composition_framebuffer,
          composition_shader,
          composition_pipeline,
          composition_material] = get_data();
  get_settings()->apply_to_material(*composition_material);

  auto* renderer_desc_set =
    get_renderer().generate_and_update_descriptor_write_sets(
      *composition_material);

  auto* material_set =
    composition_material->generate_and_update_descriptor_write_sets();

  std::array desc_sets{ renderer_desc_set, material_set };
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          composition_pipeline->get_bind_point(),
                          composition_pipeline->get_layout(),
                          0,
                          static_cast<Core::u32>(desc_sets.size()),
                          desc_sets.data(),
                          0,
                          nullptr);

  // Push constants
  const auto& pc_buffer = composition_material->get_constant_buffer();
  if (pc_buffer.valid()) {
    vkCmdPushConstants(command_buffer.get_command_buffer(),
                       composition_pipeline->get_layout(),
                       VK_SHADER_STAGE_ALL,
                       0,
                       static_cast<Core::u32>(pc_buffer.size()),
                       pc_buffer.raw());
  }

  vkCmdDraw(command_buffer.get_command_buffer(), 3, 1, 0, 0);
}

auto
CompositionRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);
}

CompositionRenderPass::CompositionSettings::CompositionSettings()
{
  dirt_texture = Renderer::get_black_texture();
}

auto
CompositionRenderPass::CompositionSettings::expose_to_ui(Material&) -> void
{
  ImGui::Text("Composition Settings");
  ImGui::DragFloat("Bloom Intensity", &Intensity, 0.05F, 0.0F, 20.0F);
  ImGui::DragFloat("Dirt Intensity", &DirtIntensity, 0.05F, 0.0F, 20.0F);
}

auto
CompositionRenderPass::CompositionSettings::apply_to_material(
  Material& material) -> void
{
  material.set("uniforms.Exposure", 0.8F);
  material.set("uniforms.Opacity", 1.0F);
  material.set("uniforms.BloomIntensity", Intensity);
  material.set("uniforms.BloomDirtIntensity", DirtIntensity);
  material.set("bloom_dirt_texture", dirt_texture);
}

} // namespace Engine::Graphics
