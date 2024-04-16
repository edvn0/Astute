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
  };

  explicit GraphicsPipeline(Configuration);
  ~GraphicsPipeline();

  auto get_pipeline() const -> VkPipeline { return pipeline; }
  auto get_layout() const -> VkPipelineLayout { return layout; }

private:
  VkPipeline pipeline{ VK_NULL_HANDLE };
  VkPipelineLayout layout{ VK_NULL_HANDLE };

  const Framebuffer* framebuffer{ nullptr };
  const Shader* shader{ nullptr };

  auto create_pipeline() -> void;
  auto create_layout() -> void;
};

} // namespace Engine::Graphics
