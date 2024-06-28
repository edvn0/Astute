#pragma once

#include "core/Maths.hpp"

#include "core/ShadowCascadeCalculator.hpp"
#include "graphics/RenderPass.hpp"

#include <span>

namespace Engine::Graphics {

static constexpr auto create_layer_views = [](auto& image) {
  auto sequence = Core::monotone_sequence<
    Core::ShadowCascadeCalculator::shadow_map_cascade_count>();
  image->create_specific_layer_image_views(std::span{ sequence });
};

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
  [[nodiscard]] auto get_extraneous_framebuffer(Core::u32 index) const
    -> const Core::Scope<IFramebuffer>& override
  {
    const auto& found = other_framebuffers.at(index);
    return found;
  }

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override {}
  auto execute_impl(CommandBuffer&) -> void override;
  auto bind(CommandBuffer&) -> void override {}
  auto unbind(CommandBuffer&) -> void override {}
  auto name() -> std::string_view override { return "Shadow"; }

private:
  Core::u32 size{ 0 };

  Core::Ref<Image> cascaded_shadow_map;
  std::vector<Core::Scope<IFramebuffer>> other_framebuffers;
  std::vector<Core::Scope<IPipeline>> other_pipelines;
};

} // namespace Engine::Graphics
