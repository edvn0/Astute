#pragma once

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class GraphicsPipeline
{
public:
  auto get_pipeline() const -> VkPipeline { return pipeline; }
  auto get_layout() const -> VkPipelineLayout { return layout; }

private:
  VkPipeline pipeline{ VK_NULL_HANDLE };
  VkPipelineLayout layout{ VK_NULL_HANDLE };
};

} // namespace Engine::Graphics
