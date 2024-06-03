#pragma once

#include "graphics/Forward.hpp"

#include "graphics/IFramebuffer.hpp"
#include "graphics/Pipeline.hpp"

using BufferBinding = Engine::Core::u32;
using BufferOffset = Engine::Core::u32;

namespace Engine::Graphics::RendererExtensions {

auto
begin_renderpass(const CommandBuffer&,
                 const IFramebuffer&,
                 bool flip = false,
                 bool primary_pass = true) -> void;
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
bind_pipeline(const CommandBuffer&, const IPipeline&) -> void;

auto
explicitly_clear_framebuffer(const CommandBuffer&,
                             const IFramebuffer&,
                             bool clear_depth = true) -> void;

}
