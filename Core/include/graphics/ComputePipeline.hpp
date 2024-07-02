#pragma once

#include "graphics/Forward.hpp"

#include "graphics/Pipeline.hpp"

namespace Engine::Graphics {

class ComputePipeline : public IPipeline
{
public:
  struct Configuration
  {
    const Shader* shader;
  };

  explicit ComputePipeline(const Configuration&);
  ~ComputePipeline() override;

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
    return VK_PIPELINE_BIND_POINT_COMPUTE;
  }

private:
  const Shader* shader{ nullptr };

  VkPipeline pipeline{ VK_NULL_HANDLE };
  VkPipelineLayout layout{ VK_NULL_HANDLE };

  auto create_pipeline() -> void;
  auto create_layout() -> void;
  auto destroy() -> void;
};

} // namespace Engine::Graphics
