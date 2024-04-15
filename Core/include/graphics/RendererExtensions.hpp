#pragma once

#include "graphics/Forward.hpp"

namespace Engine::Graphics::RendererExtensions {

auto
begin_renderpass(const CommandBuffer&, const Framebuffer&) -> void;

auto end_renderpass(const CommandBuffer&) -> void;

auto
explicitly_clear_framebuffer(const CommandBuffer&, const Framebuffer&) -> void;

}