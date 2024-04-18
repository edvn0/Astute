#pragma once

#include "graphics/Forward.hpp"

using VertexBufferBinding = Engine::Core::usize;
using VertexBufferOffset = Engine::Core::usize;

namespace Engine::Graphics::RendererExtensions {

auto
begin_renderpass(const CommandBuffer&, const Framebuffer&) -> void;
auto
end_renderpass(const CommandBuffer&) -> void;
auto
bind_vertex_buffer(const CommandBuffer&,
                   const VertexBuffer&,
                   VertexBufferBinding = 0,
                   VertexBufferOffset = 0) -> void;

auto
explicitly_clear_framebuffer(const CommandBuffer&, const Framebuffer&) -> void;

}
