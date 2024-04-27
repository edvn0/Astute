#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class PreDepthRenderPass final : public RenderPass
{
public:
  PreDepthRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
  }
  ~PreDepthRenderPass() override = default;
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto destruct_impl() -> void override {}
  auto execute_impl(CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics
