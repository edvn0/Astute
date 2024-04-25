#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class DeferredRenderPass final : public RenderPass
{
public:
  ~DeferredRenderPass() override = default;

  auto update_attachment(Core::u32 index, const Core::Ref<Image>& image)
    -> void override;
  auto construct(Renderer&) -> void override;

protected:
  auto destruct_impl() -> void override;
};

} // namespace Engine::Graphics
