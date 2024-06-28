#pragma once

#include "core/Forward.hpp"
#include "core/Serialisation.hpp"
#include "graphics/Forward.hpp"

#include "graphics/Pipeline.hpp"

namespace Engine::Graphics {

enum class Topology : Core::u8
{
  PointList = 0,
  LineList = 1,
  LineStrip = 2,
  TriangleList = 3,
  TriangleStrip = 4,
  TriangleFan = 5,
};

class GraphicsPipeline : public IPipeline
{
public:
  struct Configuration
  {
    const IFramebuffer* framebuffer;
    const Shader* shader;
    const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
    const VkCullModeFlags cull_mode{ VK_CULL_MODE_BACK_BIT };
    const VkFrontFace face_mode{ VK_FRONT_FACE_CLOCKWISE };
    const VkCompareOp depth_comparator{ VK_COMPARE_OP_GREATER_OR_EQUAL };
    const Topology topology{ Topology::TriangleList };
    const Core::f32 clear_depth_value{ 0.0F };
    const std::optional<std::vector<VkVertexInputAttributeDescription>>
      override_vertex_attributes{ std::nullopt };
    const std::optional<std::vector<VkVertexInputAttributeDescription>>
      override_instance_attributes{ std::nullopt };
  };

  explicit GraphicsPipeline(const Configuration&);
  ~GraphicsPipeline() override;

  auto on_resize(const Core::Extent&) -> void override;

  [[nodiscard]] auto get_pipeline() const -> VkPipeline override
  {
    return pipeline;
  }
  [[nodiscard]] auto get_layout() const -> VkPipelineLayout override
  {
    return layout;
  }
  [[nodiscard]] auto get_bind_point() const -> VkPipelineBindPoint override
  {
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
  }

  ASTUTE_MAKE_SERIALISABLE(GraphicsPipeline);

private:
  VkPipeline pipeline{ VK_NULL_HANDLE };
  VkPipelineLayout layout{ VK_NULL_HANDLE };
  const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  const VkCullModeFlags cull_mode{ VK_CULL_MODE_NONE };
  const VkFrontFace face_mode{ VK_FRONT_FACE_COUNTER_CLOCKWISE };
  const VkCompareOp depth_comparator{ VK_COMPARE_OP_GREATER_OR_EQUAL };
  const Topology topology{ Topology::TriangleList };
  const Core::f32 clear_depth_value{ 0.0F };
  const std::optional<std::vector<VkVertexInputAttributeDescription>>
    override_vertex_attributes;
  const std::optional<std::vector<VkVertexInputAttributeDescription>>
    override_instance_attributes;

  const IFramebuffer* framebuffer{ nullptr };
  const Shader* shader{ nullptr };

  auto create_pipeline() -> void;
  auto create_layout() -> void;
  auto destroy() -> void;
};

} // namespace Engine::Graphics
