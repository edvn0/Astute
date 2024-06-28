#include "pch/CorePCH.hpp"

#include "core/Verify.hpp"
#include "graphics/Device.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/IFramebuffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/Shader.hpp"

#include "core/Serialisation.hpp"
#include "core/YAMLFile.hpp"

#include "graphics/Vertex.hpp"

#include <glm/glm.hpp>
#include <magic_enum/magic_enum.hpp>

namespace Engine::Graphics {

namespace {
// Conversion functions
auto
to_vulkan_topology(Topology topology) -> VkPrimitiveTopology
{
  switch (topology) {
    case Topology::PointList:
      return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case Topology::LineList:
      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case Topology::LineStrip:
      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case Topology::TriangleList:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case Topology::TriangleStrip:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case Topology::TriangleFan:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
}

auto
to_vulkan_polygon_mode(Topology topology) -> VkPolygonMode
{
  switch (topology) {
    case Topology::PointList:
      return VK_POLYGON_MODE_POINT;
    default:
      return VK_POLYGON_MODE_FILL;
  }
}
}

GraphicsPipeline::GraphicsPipeline(const Configuration& config)
  : sample_count(config.sample_count)
  , cull_mode(config.cull_mode)
  , face_mode(config.face_mode)
  , depth_comparator(config.depth_comparator)
  , topology(config.topology)
  , clear_depth_value(config.clear_depth_value)
  , override_vertex_attributes(config.override_vertex_attributes)
  , override_instance_attributes(config.override_instance_attributes)
  , framebuffer(config.framebuffer)
  , shader(config.shader)
{
  create_layout();
  create_pipeline();
  trace("Created graphics pipeline for framebuffer: {}",
        framebuffer->get_name());

  Core::SerialWriter::write(*this);
}

GraphicsPipeline::~GraphicsPipeline()
{
  destroy();
}

auto
GraphicsPipeline::construct_file_path(const GraphicsPipeline& instance)
  -> std::string
{
  std::stringstream ss;
  ss << "sample_count_" << instance.sample_count << "_cull_mode_"
     << instance.cull_mode << "_face_mode_" << instance.face_mode
     << "_depth_comparator_" << instance.depth_comparator << "_topology_"
     << static_cast<int>(instance.topology) << "_clear_depth_value_"
     << instance.clear_depth_value;

  if (instance.override_vertex_attributes.has_value()) {
    ss << "_vertex_attributes_" << instance.override_vertex_attributes->size();
  }

  if (instance.override_instance_attributes.has_value()) {
    ss << "_instance_attributes_"
       << instance.override_instance_attributes->size();
  }

  // Hash the combined string to create a pseudo-unique identifier
  std::string combined = ss.str();
  static std::hash<std::string> hasher;
  auto hash = hasher(combined);

  // Create the final file path
  std::stringstream file_path;
  static constexpr auto base = std::string_view{ "Assets/pipelines" };
  file_path << std::format(
    "{}/graphics_pipeline_{}_{}.yaml", base, instance.shader->get_name(), hash);
  return file_path.str();
}

auto
GraphicsPipeline::write(const GraphicsPipeline& instance,
                        std::ostream& output_file) -> bool
{
  YAML::Node node;
  node["sample_count"] = static_cast<Core::u32>(instance.sample_count);
  node["cull_mode"] = static_cast<Core::u32>(instance.cull_mode);
  node["face_mode"] = magic_enum::enum_name(instance.face_mode);
  node["depth_comparator"] = magic_enum::enum_name(instance.depth_comparator);
  node["topology"] = magic_enum::enum_name(instance.topology);
  node["clear_depth_value"] =
    static_cast<Core::f32>(instance.clear_depth_value);

  if (instance.override_vertex_attributes.has_value()) {
    YAML::Node vertex_attributes = YAML::Node(YAML::NodeType::Sequence);
    for (const auto& attribute : *instance.override_vertex_attributes) {
      YAML::Node attr_node;
      attr_node["location"] = attribute.location;
      attr_node["binding"] = attribute.binding;
      attr_node["format"] = magic_enum::enum_name(attribute.format);
      attr_node["offset"] = attribute.offset;
      vertex_attributes.push_back(attr_node);
    }
    node["override_vertex_attributes"] = vertex_attributes;
  }

  if (instance.override_instance_attributes.has_value()) {
    YAML::Node instance_attributes = YAML::Node(YAML::NodeType::Sequence);
    for (const auto& attribute : *instance.override_instance_attributes) {
      YAML::Node attr_node;
      attr_node["location"] = attribute.location;
      attr_node["binding"] = attribute.binding;
      attr_node["format"] = magic_enum::enum_name(attribute.format);
      attr_node["offset"] = attribute.offset;
      instance_attributes.push_back(attr_node);
    }
    node["override_instance_attributes"] = instance_attributes;
  }

  try {
    output_file << node;
  } catch (const std::ios_base::failure& e) {
    error(e);
    return false;
  }

  return true;
}

auto
GraphicsPipeline::read(GraphicsPipeline&, const std::istream&) -> bool
{
  return true;
}

auto
GraphicsPipeline::destroy() -> void
{
  vkDestroyPipeline(Device::the().device(), pipeline, nullptr);
  vkDestroyPipelineLayout(Device::the().device(), layout, nullptr);
}

auto
GraphicsPipeline::on_resize(const Core::Extent&) -> void
{
  destroy();
  create_layout();
  create_pipeline();
}

auto
GraphicsPipeline::create_pipeline() -> void
{
  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  VkPipelineViewportStateCreateInfo viewport_state_info{};
  viewport_state_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_info.viewportCount = 1;
  viewport_state_info.scissorCount = 1;
  pipeline_info.pViewportState = &viewport_state_info;

  const auto maybe_vertex_stage =
    shader->get_shader_module(Shader::Type::Vertex);
  const auto maybe_fragment_stage =
    shader->get_shader_module(Shader::Type::Fragment);

  if (!maybe_vertex_stage.has_value() || !maybe_fragment_stage.has_value()) {
    throw std::runtime_error("Failed to get shader modules");
  }

  VkSpecializationMapEntry specialization_entry{};
  specialization_entry.constantID = 0;
  specialization_entry.offset = 0;
  specialization_entry.size = sizeof(uint32_t);

#ifndef ASTUTE_PERFORMANCE
  Core::u32 specialisation_data = sample_count;
#else
  Core::u32 specialisation_data = 1;
#endif

  VkSpecializationInfo specialisation_info{};
  specialisation_info.mapEntryCount = 1;
  specialisation_info.pMapEntries = &specialization_entry;
  specialisation_info.dataSize = sizeof(specialisation_data);
  specialisation_info.pData = &specialisation_data;

  std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
    VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = maybe_vertex_stage.value(),
      .pName = "main",
      .pSpecializationInfo = &specialisation_info,
    },
    VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = maybe_fragment_stage.value(),
      .pName = "main",
      .pSpecializationInfo = &specialisation_info,
    },
  };
  pipeline_info.stageCount = static_cast<Core::u32>(shader_stages.size());
  pipeline_info.pStages = shader_stages.data();

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  std::vector<VkVertexInputBindingDescription> binding_descriptions;
  auto& vertex_binding = binding_descriptions.emplace_back();
  vertex_binding.binding = 0;
  vertex_binding.stride = sizeof(Vertex);
  vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  auto attributes = generate_vertex_attributes();
  if (override_vertex_attributes.has_value()) {
    attributes = override_vertex_attributes.value();
  }

  auto instance_attributes = generate_instance_attributes();
  if (override_instance_attributes.has_value()) {
    instance_attributes = override_instance_attributes.value();
  } else {
    auto& instance_binding = binding_descriptions.emplace_back();
    instance_binding.binding = 1;
    instance_binding.stride = 3 * sizeof(glm::vec4);
    instance_binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
  }

  std::vector<VkVertexInputAttributeDescription> all_attributes;
  all_attributes.insert(
    all_attributes.end(), attributes.begin(), attributes.end());
  all_attributes.insert(all_attributes.end(),
                        instance_attributes.begin(),
                        instance_attributes.end());

  vertex_input_info.vertexAttributeDescriptionCount =
    static_cast<Core::u32>(all_attributes.size());
  vertex_input_info.pVertexAttributeDescriptions = all_attributes.data();
  pipeline_info.pVertexInputState = &vertex_input_info;
  vertex_input_info.vertexBindingDescriptionCount =
    static_cast<Core::u32>(binding_descriptions.size());
  vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();

  if ((override_instance_attributes.has_value() &&
       override_instance_attributes->empty()) &&
      (override_vertex_attributes.has_value() &&
       override_vertex_attributes->empty())) {
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
  }

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
  input_assembly_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.topology = to_vulkan_topology(topology);
  input_assembly_info.primitiveRestartEnable = VK_FALSE;
  pipeline_info.pInputAssemblyState = &input_assembly_info;

  VkPipelineRasterizationStateCreateInfo rasterization_info{};
  rasterization_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_info.depthClampEnable = VK_TRUE;
  rasterization_info.rasterizerDiscardEnable = VK_FALSE;
  rasterization_info.polygonMode = to_vulkan_polygon_mode(topology);
  rasterization_info.lineWidth = 1.0F;
  rasterization_info.cullMode = cull_mode;
  rasterization_info.frontFace = face_mode;
  if ((framebuffer != nullptr) && framebuffer->has_depth_attachment()) {
    rasterization_info.depthBiasEnable =
      framebuffer->get_depth_attachment()->configuration.format ==
          VK_FORMAT_D32_SFLOAT
        ? VK_FALSE
        : VK_TRUE;
  }
  rasterization_info.depthBiasConstantFactor = 0.0F;
  rasterization_info.depthBiasClamp = 0.0F;
  rasterization_info.depthBiasSlopeFactor = 0.0F;
  pipeline_info.pRasterizationState = &rasterization_info;

  VkPipelineMultisampleStateCreateInfo multisample_info{};
  multisample_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_info.sampleShadingEnable =
    sample_count != VK_SAMPLE_COUNT_1_BIT ? VK_TRUE : VK_FALSE;
  multisample_info.rasterizationSamples = sample_count;

  pipeline_info.pMultisampleState = &multisample_info;

  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
  depth_stencil_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = depth_comparator;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.stencilTestEnable = VK_FALSE;
  depth_stencil_info.back.failOp = VK_STENCIL_OP_KEEP;
  depth_stencil_info.back.passOp = VK_STENCIL_OP_KEEP;
  depth_stencil_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depth_stencil_info.front = depth_stencil_info.back;
  pipeline_info.pDepthStencilState = &depth_stencil_info;

  VkPipelineColorBlendStateCreateInfo color_blend_info{};
  color_blend_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_TRUE;
  color_blend_info.logicOp = VK_LOGIC_OP_AND;

  auto color_blend_states = framebuffer->construct_blend_states();
  color_blend_info.attachmentCount =
    static_cast<Core::u32>(color_blend_states.size());
  color_blend_info.pAttachments = color_blend_states.data();
  pipeline_info.pColorBlendState = &color_blend_info;

  VkPipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  std::array dynamic_states = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_DEPTH_BIAS,
    VK_DYNAMIC_STATE_LINE_WIDTH,
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
