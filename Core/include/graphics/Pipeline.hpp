#pragma once

#include "core/Types.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

struct IPipeline
{
  IPipeline() = default;
  virtual ~IPipeline();

  virtual auto on_resize(const Core::Extent&) -> void = 0;
  [[nodiscard]] virtual auto get_pipeline() const -> VkPipeline = 0;
  [[nodiscard]] virtual auto get_layout() const -> VkPipelineLayout = 0;
  [[nodiscard]] virtual auto get_bind_point() const -> VkPipelineBindPoint = 0;

  auto get_pipeline_cache() -> VkPipelineCache { return pipeline_cache; }

protected:
  void create_pipeline_cache(const std::string& cache_filename);
  void save_pipeline_cache(const std::string& cache_filename);

private:
  VkPipelineCache pipeline_cache{ nullptr };

  void create_cache(const std::string& cache_filename);
  void save_cache(const std::string& cache_filename);
};

} // namespace Engine::Graphics
