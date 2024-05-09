#pragma once

#include "graphics/Forward.hpp"

#include "graphics/IFramebuffer.hpp"

using BufferBinding = Engine::Core::usize;
using BufferOffset = Engine::Core::usize;

namespace Engine::Graphics::RendererExtensions {

auto
begin_renderpass(const CommandBuffer&, const IFramebuffer&) -> void;
auto
end_renderpass(const CommandBuffer&) -> void;
auto
bind_vertex_buffer(const CommandBuffer&,
                   const VertexBuffer&,
                   BufferBinding = 0,
                   BufferOffset = 0) -> void;
auto
bind_index_buffer(const CommandBuffer&,
                  const IndexBuffer&,
                  BufferBinding = 0,
                  BufferOffset = 0) -> void;

auto
explicitly_clear_framebuffer(const CommandBuffer&,
                             const IFramebuffer&,
                             bool clear_depth = true) -> void;

}
