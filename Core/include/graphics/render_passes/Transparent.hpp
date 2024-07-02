#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class TransparentRenderPass final : public RenderPass
{
public:
  explicit TransparentRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
  }
  ~TransparentRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;
  auto name() -> std::string_view override { return "Transparent"; }

private:
  Core::Ref<Image> depth_attachment;
};

} // namespace Engine::Graphics
