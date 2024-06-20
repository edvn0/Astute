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
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
  auto name() -> std::string_view override { return "Predepth"; }
};

} // namespace Engine::Graphics
