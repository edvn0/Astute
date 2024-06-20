#include "imgui.h"
#include "pch/CorePCH.hpp"

#include "graphics/render_passes/ChromaticAberration.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Image.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"

#include "ui/UI.hpp"

namespace Engine::Graphics {

auto
ChromaticAberrationRenderPass::construct_impl() -> void
{
  const auto& ext = get_renderer().get_size();
  auto&& [chromatic_aberration_framebuffer,
          chromatic_aberration_shader,
          chromatic_aberration_pipeline,
          chromatic_aberration_material] = get_data();
  chromatic_aberration_framebuffer =
    Core::make_scope<Framebuffer>(FramebufferSpecification{
      .width = ext.width,
      .height = ext.height,
      .attachments = { { .format = VK_FORMAT_R32G32B32A32_SFLOAT, }, },
      .debug_name = "ChromaticAberration",
    });

  chromatic_aberration_shader =
    Shader::compile_graphics_scoped("Assets/shaders/chromatic_aberration.vert",
                                    "Assets/shaders/chromatic_aberration.frag");
  chromatic_aberration_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = chromatic_aberration_framebuffer.get(),
      .shader = chromatic_aberration_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_LESS,
      .override_vertex_attributes = {
          {  },
        },
      .override_instance_attributes = {
          {  },
        },
    });

  chromatic_aberration_material =
    Core::make_scope<Material>(Material::Configuration{
      .shader = chromatic_aberration_shader.get(),
    });
  const auto& input_render_pass = get_renderer().get_render_pass("Deferred");
  chromatic_aberration_material->set(
    "fullscreen_texture", input_render_pass.get_colour_attachment(0));
  get_settings()->apply_to_material(*chromatic_aberration_material);
}

auto
ChromaticAberrationRenderPass::execute_impl(CommandBuffer& command_buffer)
  -> void
{
  ASTUTE_PROFILE_FUNCTION();
  auto&& [chromatic_aberration_framebuffer,
          chromatic_aberration_shader,
          chromatic_aberration_pipeline,
          chromatic_aberration_material] = get_data();

  auto* renderer_desc_set =
    get_renderer().generate_and_update_descriptor_write_sets(
      *chromatic_aberration_material);

  auto* material_set =
    chromatic_aberration_material->generate_and_update_descriptor_write_sets();

  std::array desc_sets{ renderer_desc_set, material_set };
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          chromatic_aberration_pipeline->get_bind_point(),
                          chromatic_aberration_pipeline->get_layout(),
                          0,
                          static_cast<Core::u32>(desc_sets.size()),
                          desc_sets.data(),
                          0,
                          nullptr);

  // Push constants
  const auto& pc_buffer = chromatic_aberration_material->get_constant_buffer();
  vkCmdPushConstants(command_buffer.get_command_buffer(),
                     chromatic_aberration_pipeline->get_layout(),
                     VK_SHADER_STAGE_ALL,
                     0,
                     static_cast<Core::u32>(pc_buffer.size()),
                     pc_buffer.raw());

  vkCmdDraw(command_buffer.get_command_buffer(), 3, 1, 0, 0);
}

auto
ChromaticAberrationRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);
}

auto
ChromaticAberrationRenderPass::ChromaticAberrationSettings::expose_to_ui(
  Material& material) -> void
{
  ImGui::Text("Chromatic Aberration Settings");
  if (ImGui::SliderFloat3("Intensity",
                          glm::value_ptr(chromatic_aberration),
                          0.0001F,
                          0.05F,
                          "%.4f")) {
    material.set("uniforms.aberration_offset", chromatic_aberration);
  }
}

auto
ChromaticAberrationRenderPass::ChromaticAberrationSettings::apply_to_material(
  Material& material) -> void
{
  material.set("uniforms.aberration_offset", chromatic_aberration);
}

} // namespace Engine::Graphics
