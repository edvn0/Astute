#include "pch/CorePCH.hpp"

#include "graphics/RenderPass.hpp"

#include "graphics/Framebuffer.hpp"

namespace Engine::Graphics {

auto
RenderPass::construct(Renderer&) -> void
{
  // Should be implemented by derived classes
}

auto
RenderPass::destruct_impl() -> void
{
  // Should be implemented by derived classes
}

auto
RenderPass::update_attachment(Core::u32 index, const Core::Ref<Image>& image) -> void
{
  framebuffer->update_attachment(index, image);
}

} // namespace Engine::Graphic
