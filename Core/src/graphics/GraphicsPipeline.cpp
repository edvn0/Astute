#include "pch/CorePCH.hpp"

#include "core/Verify.hpp"
#include "graphics/Device.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Shader.hpp"

#include "graphics/Vertex.hpp"

#include <glm/glm.hpp>

namespace Engine::Graphics {

GraphicsPipeline::GraphicsPipeline(Configuration config)
  : sample_count(config.sample_count)
  , framebuffer(config.framebuffer)
  , shader(config.shader)
{
  create_layout();
  create_pipeline();
}

GraphicsPipeline::~GraphicsPipeline()
{
  vkDestroyPipeline(Device::the().device(), pipeline, nullptr);
  vkDestroyPipelineLayout(Device::the().device(), layout, nullptr);
}

auto
GraphicsPipeline::create_pipeline() -> void
{
  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(framebuffer->get_extent().width);
  viewport.height = static_cast<float>(framebuffer->get_extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = framebuffer->get_extent();

  VkPipelineViewportStateCreateInfo viewport_state_info{};
  viewport_state_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_info.viewportCount = 1;
  viewport_state_info.pViewports = &viewport;
  viewport_state_info.scissorCount = 1;
  viewport_state_info.pScissors = &scissor;
  pipeline_info.pViewportState = &viewport_state_info;

  const auto maybe_vertex_stage =
    shader->get_shader_module(Shader::Type::Vertex);
  const auto maybe_fragment_stage =
    shader->get_shader_module(Shader::Type::Fragment);

  if (!maybe_vertex_stage.has_value() || !maybe_fragment_stage.has_value()) {
    throw std::runtime_error("Failed to get shader modules");
  }

  std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
    VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = maybe_vertex_stage.value(),
      .pName = "main",
    },
    VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = maybe_fragment_stage.value(),
      .pName = "main",
    },
  };
  pipeline_info.stageCount = static_cast<Core::u32>(shader_stages.size());
  pipeline_info.pStages = shader_stages.data();

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  std::array<VkVertexInputBindingDescription, 2> binding_descriptions = {};
  binding_descriptions[0].binding = 0;
  binding_descriptions[0].stride = sizeof(Vertex);
  binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  binding_descriptions[1].binding = 1;
  binding_descriptions[1].stride = sizeof(glm::mat4);
  binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  vertex_input_info.vertexBindingDescriptionCount =
    static_cast<Core::u32>(binding_descriptions.size());
  vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();

  auto attributes = generate_attributes();
  vertex_input_info.vertexAttributeDescriptionCount =
    static_cast<Core::u32>(attributes.size());
  vertex_input_info.pVertexAttributeDescriptions = attributes.data();
  pipeline_info.pVertexInputState = &vertex_input_info;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
  input_assembly_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_info.primitiveRestartEnable = VK_FALSE;
  pipeline_info.pInputAssemblyState = &input_assembly_info;

  VkPipelineRasterizationStateCreateInfo rasterization_info{};
  rasterization_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_info.depthClampEnable = VK_FALSE;
  rasterization_info.rasterizerDiscardEnable = VK_FALSE;
  rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_info.lineWidth = 1.0f;
  rasterization_info.cullMode = VK_CULL_MODE_FRONT_BIT;
  rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_info.depthBiasEnable = VK_FALSE;
  rasterization_info.depthBiasConstantFactor = 0.0f;
  rasterization_info.depthBiasClamp = 0.0f;
  rasterization_info.depthBiasSlopeFactor = 0.0f;
  pipeline_info.pRasterizationState = &rasterization_info;

  VkPipelineMultisampleStateCreateInfo multisample_info{};
  multisample_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_info.sampleShadingEnable = VK_FALSE;
  multisample_info.rasterizationSamples = sample_count;

  pipeline_info.pMultisampleState = &multisample_info;

  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
  depth_stencil_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.stencilTestEnable = VK_FALSE;
  pipeline_info.pDepthStencilState = &depth_stencil_info;

  VkPipelineColorBlendStateCreateInfo color_blend_info{};
  color_blend_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_COPY;

  auto color_blend_states = framebuffer->construct_blend_states();
  color_blend_info.attachmentCount =
    static_cast<Core::u32>(color_blend_states.size());
  color_blend_info.pAttachments = color_blend_states.data();
  pipeline_info.pColorBlendState = &color_blend_info;

  VkPipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  std::array<VkDynamicState, 2> dynamic_states = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  dynamic_state_info.dynamicStateCount =
    static_cast<Core::u32>(dynamic_states.size());
  dynamic_state_info.pDynamicStates = dynamic_states.data();
  pipeline_info.pDynamicState = &dynamic_state_info;

  pipeline_info.layout = layout;
  pipeline_info.renderPass = framebuffer->get_renderpass();
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(Device::the().device(),
                                     VK_NULL_HANDLE,
                                     1,
                                     &pipeline_info,
                                     nullptr,
                                     &pipeline));
}

auto
GraphicsPipeline::create_layout() -> void
{
  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

  auto descriptor_set_layouts = shader->get_descriptor_set_layouts();
  layout_info.setLayoutCount =
    static_cast<Core::u32>(descriptor_set_layouts.size());
  layout_info.pSetLayouts = descriptor_set_layouts.data();
  layout_info.pushConstantRangeCount = 0;
  layout_info.pPushConstantRanges = nullptr;

  VK_CHECK(vkCreatePipelineLayout(
    Device::the().device(), &layout_info, nullptr, &layout));
}

}