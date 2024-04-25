#pragma once

#include "graphics/Forward.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class GraphicsPipeline
{
public:
  struct Configuration
  {
    const Framebuffer* framebuffer;
    const Shader* shader;
    const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
    const VkCullModeFlags cull_mode{ VK_CULL_MODE_BACK_BIT };
    const VkFrontFace face_mode{ VK_FRONT_FACE_COUNTER_CLOCKWISE };
    const std::optional<std::vector<VkVertexInputAttributeDescription>>
      override_vertex_attributes{};
    const std::optional<std::vector<VkVertexInputAttributeDescription>>
      override_instance_attributes{};
  };

  explicit GraphicsPipeline(const Configuration&);
  ~GraphicsPipeline();

  auto on_resize(const Core::Extent&) -> void;

  auto get_pipeline() const -> VkPipeline { return pipeline; }
  auto get_layout() const -> VkPipelineLayout { return layout; }

private:
  VkPipeline pipeline{ VK_NULL_HANDLE };
  VkPipelineLayout layout{ VK_NULL_HANDLE };
  const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  const VkCullModeFlags cull_mode{ VK_CULL_MODE_NONE };
  const VkFrontFace face_mode{ VK_FRONT_FACE_CLOCKWISE };
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
