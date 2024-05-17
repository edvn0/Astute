#pragma once

#include "core/Types.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

struct IPipeline
{
  virtual ~IPipeline() = default;

  virtual auto on_resize(const Core::Extent&) -> void = 0;
  virtual auto get_pipeline() const -> VkPipeline = 0;
  virtual auto get_layout() const -> VkPipelineLayout = 0;
  virtual auto get_bind_point () const -> VkPipelineBindPoint = 0;
};

} // namespace Engine::Graphics
