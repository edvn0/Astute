#include "pch/CorePCH.hpp"

#include "graphics/render_passes/LightCulling.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "core/Application.hpp"
#include "graphics/ComputePipeline.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
LightCullingRenderPass::construct() -> void
{
  on_resize(get_renderer().get_size());
}

auto
LightCullingRenderPass::execute_compute_impl(CommandBuffer& command_buffer)
  -> void
{
  auto& [light_culling_framebuffer,
         light_culling_shader,
         light_culling_pipeline,
         light_culling_material] = get_data();

  light_culling_material->set(
    "predepth_map",
    get_renderer().get_render_pass("Predepth").get_depth_attachment(true));

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(*light_culling_material);

  auto light_culling_descriptor_set =
    light_culling_material->generate_and_update_descriptor_write_sets();

  std::array descriptor_sets = { descriptor_set, light_culling_descriptor_set };
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          light_culling_pipeline->get_bind_point(),
                          light_culling_pipeline->get_layout(),
                          0,
                          static_cast<Core::u32>(descriptor_sets.size()),
                          descriptor_sets.data(),
                          0,
                          nullptr);

  vkCmdDispatch(command_buffer.get_command_buffer(),
                workgroups.x,
                workgroups.y,
                workgroups.z);
}

auto
LightCullingRenderPass::destruct_impl() -> void
{
}

auto
LightCullingRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [_,
          light_culling_shader,
          light_culling_pipeline,
          light_culling_material] = get_data();
  light_culling_shader =
    Shader::compile_compute_scoped("Assets/shaders/light_culling.comp");

  if (!light_culling_shader) {
    error("Failed to compile light culling shader");
    return;
  }

  light_culling_pipeline =
    Core::make_scope<ComputePipeline>(ComputePipeline::Configuration{
      .shader = light_culling_shader.get(),
    });
  light_culling_material = Core::make_scope<Material>(Material::Configuration{
    .shader = light_culling_shader.get(),
  });
}

} // namespace Engine::Graphics
