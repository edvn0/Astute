#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class PreDepthRenderPass final : public RenderPass
{
public:
  ~PreDepthRenderPass() override = default;
  auto construct(Renderer&) -> void override;
  auto on_resize(Renderer&, const Core::Extent&) -> void override {}

protected:
  auto destruct_impl() -> void override {}
  auto execute_impl(Renderer&, CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics
