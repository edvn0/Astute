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

static constexpr auto emplace_transform = [](auto& transform_dict,
                                             const auto& matrix) {
  auto& mesh_transform = transform_dict.transforms.emplace_back();
  mesh_transform.transform_rows[0] = {
    matrix[0][0],
    matrix[1][0],
    matrix[2][0],
    matrix[3][0],
  };
  mesh_transform.transform_rows[1] = {
    matrix[0][1],
    matrix[1][1],
    matrix[2][1],
    matrix[3][1],
  };
  mesh_transform.transform_rows[2] = {
    matrix[0][2],
    matrix[1][2],
    matrix[2][2],
    matrix[3][2],
  };
};

static constexpr auto update_lights = []<class Light>(Light& light_ubo,
                                                      auto& env_lights) {
  auto i = 0ULL;
  auto& [ubo_count, ubo_lights] = light_ubo.get_data();
  ubo_count = static_cast<Core::i32>(env_lights.size());
  for (const auto& light : env_lights) {
    ubo_lights.at(i) = light;
    i++;
  }
  light_ubo.update();
};

}
