#include "pch/CorePCH.hpp"

#include "graphics/ComputePipeline.hpp"

#include "core/Verify.hpp"
#include "graphics/Device.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Shader.hpp"

#include "graphics/Vertex.hpp"

#include <glm/glm.hpp>

namespace Engine::Graphics {

ComputePipeline::ComputePipeline(const Configuration& config)
  : shader(config.shader)
{
  create_layout();
  create_pipeline();
}

ComputePipeline::~ComputePipeline()
{
  destroy();
}

auto
ComputePipeline::destroy() -> void
{
  vkDestroyPipeline(Device::the().device(), pipeline, nullptr);
  vkDestroyPipelineLayout(Device::the().device(), layout, nullptr);
}

auto
ComputePipeline::on_resize(const Core::Extent&) -> void
{
  destroy();
  create_layout();
  create_pipeline();
}

auto
ComputePipeline::create_pipeline() -> void
{

  VkComputePipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.layout = layout;

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module =
    shader->get_shader_module(Shader::Type::Compute).value();
  shader_stage_info.pName = "main";

  pipeline_info.stage = shader_stage_info;

  VK_CHECK(vkCreateComputePipelines(Device::the().device(),
                                    VK_NULL_HANDLE,
                                    1,
                                    &pipeline_info,
                                    nullptr,
                                    &pipeline));
}

auto
ComputePipeline::create_layout() -> void
{
  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

  const auto& descriptor_set_layouts = shader->get_descriptor_set_layouts();
  layout_info.setLayoutCount =
    static_cast<Core::u32>(descriptor_set_layouts.size());
  layout_info.pSetLayouts = descriptor_set_layouts.data();

  const auto& pcs_from_shader =
    shader->get_reflection_data().push_constant_ranges;
  std::vector<VkPushConstantRange> vk_pc_range(pcs_from_shader.size());
  for (Core::u32 i = 0; i < pcs_from_shader.size(); i++) {
    const auto& pc_range = pcs_from_shader[i];
    auto& vulkan_pc_range = vk_pc_range[i];

    vulkan_pc_range.stageFlags = pc_range.shader_stage;
    vulkan_pc_range.offset = pc_range.offset;
    vulkan_pc_range.size = pc_range.size;
  }
  layout_info.pushConstantRangeCount =
    static_cast<Core::u32>(vk_pc_range.size());
  layout_info.pPushConstantRanges = vk_pc_range.data();

  VK_CHECK(vkCreatePipelineLayout(
    Device::the().device(), &layout_info, nullptr, &layout));
}

}
