#pragma once

#include "graphics/RenderPass.hpp"

#include <glm/glm.hpp>

namespace Engine::Graphics {

class LightCullingRenderPass final : public RenderPass
{
public:
  explicit LightCullingRenderPass(Renderer& ren, const glm::uvec3& wgs)
    : RenderPass(ren)
    , workgroups(wgs)
  {
  }
  ~LightCullingRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
  auto is_valid() const -> bool override
  {
    auto&& [_, shader, pipeline, material] = get_data();
    return shader && pipeline && material;
  }

private:
  const glm::uvec3& workgroups;
  Core::Ref<Image> light_culling_image;
};

} // namespace Engine::Graphics
