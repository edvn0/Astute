#pragma once

#include "graphics/RenderPass.hpp"

namespace filewatch {
template<typename T>
class FileWatch;
}

namespace Engine::Graphics {

class DeferredRenderPass final : public RenderPass
{
public:
  explicit DeferredRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
  }
  ~DeferredRenderPass() override = default;
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;

private:
  Core::Scope<filewatch::FileWatch<std::string>> watch;
};

} // namespace Engine::Graphics
