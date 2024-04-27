#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class ShadowRenderPass final : public RenderPass
{
public:
  explicit ShadowRenderPass(Renderer& renderer, Core::u32 map_size)
    : RenderPass(renderer)
    , size(map_size)
  {
  }
  ~ShadowRenderPass() override = default;
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto destruct_impl() -> void override {}
  auto execute_impl(CommandBuffer&) -> void override;

private:
  Core::u32 size{ 0 };
};

} // namespace Engine::Graphics
