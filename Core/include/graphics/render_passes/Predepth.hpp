#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class PredepthRenderPass final : public RenderPass
{
public:
  explicit PredepthRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
  }
  ~PredepthRenderPass() override = default;
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics
