#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class DeferredRenderPass final : public RenderPass
{
public:
  ~DeferredRenderPass() override = default;
  auto construct(Renderer&) -> void override;
  auto on_resize(Renderer&, const Core::Extent&) -> void override {}

protected:
  auto destruct_impl() -> void override;
  auto execute_impl(Renderer&, CommandBuffer&) -> void override;
};

} // namespace Engine::Graphics