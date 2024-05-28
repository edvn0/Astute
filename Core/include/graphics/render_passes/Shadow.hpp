#pragma once

#include "graphics/RenderPass.hpp"

namespace Engine::Graphics {

class ShadowRenderPass final : public RenderPass
{
public:
  explicit ShadowRenderPass(Renderer& renderer, Core::u32 map_size)
    : RenderPass(renderer)
    , size(map_size)
  {
  }
  ~ShadowRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;
  auto get_extraneous_framebuffer(Core::u32 index)
    -> Core::Scope<IFramebuffer>& override
  {
    auto& found = other_framebuffers.at(index);
    return found;
  }
  auto get_extraneous_framebuffer(Core::u32 index) const
    -> const Core::Scope<IFramebuffer>& override
  {
    auto& found = other_framebuffers.at(index);
    return found;
  }

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override {}
  auto execute_impl(CommandBuffer&) -> void override;
  auto bind(CommandBuffer&) -> void override {}
  auto unbind(CommandBuffer&) -> void override {}

private:
  Core::u32 size{ 0 };

  Core::Ref<Image> cascaded_shadow_map;
  std::vector<Core::Scope<IFramebuffer>> other_framebuffers;
  std::vector<Core::Scope<IPipeline>> other_pipelines;
};

} // namespace Engine::Graphics
