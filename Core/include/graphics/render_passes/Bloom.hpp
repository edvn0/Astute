#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class BloomRenderPass final : public RenderPass
{
public:
  explicit BloomRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
    create_settings<BloomSettings>();
  }
  ~BloomRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

  auto get_bloom_texture_output() const -> const auto&
  {
    return bloom_chain.at(2);
  }

private:
  auto construct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
  auto is_valid() const -> bool override
  {
    auto&& [_, shader, pipeline, material] = get_data();
    return shader && pipeline && material;
  }

  std::array<Core::Ref<Image>, 3> bloom_chain;

  class BloomSettings : public RenderPassSettings
  {
  public:
    Core::f32 Threshold = 1.0f;
    Core::f32 Knee = 0.1f;
    Core::u32 bloom_workgroup_size{ 4 };

    auto expose_to_ui(Material&) -> void override;
    auto apply_to_material(Material&) -> void override;
  };
};

} // namespace Engine::Graphics
