#pragma once

#include "graphics/Forward.hpp"

#include "graphics/Pipeline.hpp"

namespace Engine::Graphics {

class GraphicsPipeline : public IPipeline
{
public:
  struct Configuration
  {
    const Framebuffer* framebuffer;
    const Shader* shader;
    const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
    const VkCullModeFlags cull_mode{ VK_CULL_MODE_BACK_BIT };
    const VkFrontFace face_mode{ VK_FRONT_FACE_COUNTER_CLOCKWISE };
    const VkCompareOp depth_comparator{ VK_COMPARE_OP_GREATER_OR_EQUAL };
    const Core::f32 clear_depth_value{ 0.0F };
    const std::optional<std::vector<VkVertexInputAttributeDescription>>
      override_vertex_attributes{};
    const std::optional<std::vector<VkVertexInputAttributeDescription>>
      override_instance_attributes{};
  };

  explicit GraphicsPipeline(const Configuration&);
  ~GraphicsPipeline() override;

  auto on_resize(const Core::Extent&) -> void override;

  auto get_pipeline() const -> VkPipeline override { return pipeline; }
  auto get_layout() const -> VkPipelineLayout override { return layout; }
  auto get_bind_point() const -> VkPipelineBindPoint override
  {
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
  }

private:
  VkPipeline pipeline{ VK_NULL_HANDLE };
  VkPipelineLayout layout{ VK_NULL_HANDLE };
  const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  const VkCullModeFlags cull_mode{ VK_CULL_MODE_NONE };
  const VkFrontFace face_mode{ VK_FRONT_FACE_COUNTER_CLOCKWISE };
  const VkCompareOp depth_comparator{ VK_COMPARE_OP_GREATER_OR_EQUAL };
  const Core::f32 clear_depth_value{ 0.0F };
  const std::optional<std::vector<VkVertexInputAttributeDescription>>
    override_vertex_attributes{};
  const std::optional<std::vector<VkVertexInputAttributeDescription>>
    override_instance_attributes{};

  const Framebuffer* framebuffer{ nullptr };
  const Shader* shader{ nullptr };

  auto create_pipeline() -> void;
  auto create_layout() -> void;
  auto destroy() -> void;
};

} // namespace Engine::Graphics
