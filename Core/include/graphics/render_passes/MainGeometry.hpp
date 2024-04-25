#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class MainGeometryRenderPass final : public RenderPass
{
public:
  ~MainGeometryRenderPass() override = default;
  auto construct(Renderer&) -> void override;
  auto on_resize(Renderer&, const Core::Extent&) -> void override {}

protected:
  auto destruct_impl() -> void override;
  auto execute_impl(Renderer&, CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics
