#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class LightsRenderPass final : public RenderPass
{
public:
  explicit LightsRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
  }
  ~LightsRenderPass() override = default;
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto bind(CommandBuffer&) -> void override;
  auto unbind(CommandBuffer&) -> void override;
  auto destruct_impl() -> void override {}
  auto execute_impl(CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics
