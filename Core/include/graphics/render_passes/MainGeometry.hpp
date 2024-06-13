#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class MainGeometryRenderPass final : public RenderPass
{
public:
  explicit MainGeometryRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
  }
  ~MainGeometryRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics
