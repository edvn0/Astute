#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class ChromaticAberrationRenderPass final : public RenderPass
{
public:
  explicit ChromaticAberrationRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
    create_settings<ChromaticAberrationSettings>();
  }
  ~ChromaticAberrationRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

private:
  auto construct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;

  class ChromaticAberrationSettings : public RenderPassSettings
  {
  public:
    Core::f32 chromatic_aberration = 0.001f;

    auto expose_to_ui(Material&) -> void override;
    auto apply_to_material(Material&) -> void override;
  };
};

} // namespace Engine::Graphics
