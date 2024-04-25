#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class ShadowRenderPass final : public RenderPass
{
public:
  explicit ShadowRenderPass(Core::u32 map_size)
    : size(map_size)
  {
  }
  ~ShadowRenderPass() override = default;
  auto construct(Renderer&) -> void override;
  auto on_resize(Renderer&, const Core::Extent&) -> void override {}

protected:
  auto destruct_impl() -> void override {}
  auto execute_impl(Renderer&, CommandBuffer&) -> void override;

private:
  Core::u32 size{ 0 };
};

} // namespace Engine::Graphics
