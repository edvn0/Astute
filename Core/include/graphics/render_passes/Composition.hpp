#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class CompositionRenderPass final : public RenderPass
{
public:
  explicit CompositionRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
    create_settings<CompositionSettings>();
  }
  ~CompositionRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

private:
  auto construct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
  auto name() -> std::string_view override { return "Composition"; }

  class CompositionSettings : public RenderPassSettings
  {
  public:
    bool Enabled = true;
    Core::f32 Threshold = 1.0F;
    Core::f32 Knee = 0.1F;
    Core::f32 UpsampleScale = 1.0F;
    Core::f32 Intensity = 1.0F;
    Core::f32 DirtIntensity = 1.0F;
    Core::Ref<Image> dirt_texture{ nullptr };

    CompositionSettings();

    auto expose_to_ui(Material&) -> void override;
    auto apply_to_material(Material&) -> void override;
  };
};

} // namespace Engine::Graphics
