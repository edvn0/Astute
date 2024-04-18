#pragma once

#include "core/DataBuffer.hpp"
#include "core/Forward.hpp"
#include "core/Types.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"
#include "graphics/Shader.hpp"

#include "graphics/ShaderBuffers.hpp"

#include <glm/glm.hpp>
#include <string_view>

namespace Engine::Graphics {
struct CommandKey;
}

template<>
struct std::hash<Engine::Graphics::CommandKey>
{
  auto operator()(const Engine::Graphics::CommandKey& key) const noexcept
    -> Engine::Core::usize;
}; // namespace std

namespace Engine::Graphics {

struct TransformVertexData
{
  std::array<glm::vec4, 4> transform_rows{};
};
struct TransformMapData
{
  std::vector<TransformVertexData> transforms;
  Core::u32 offset = 0;
};
struct SubmeshTransformBuffer
{
  Core::Scope<UniformBufferObject<TransformMapData, GPUBufferType::Vertex>>
    transform_buffer{ nullptr };
  Core::Scope<Core::DataBuffer> data_buffer{ nullptr };
};

struct RenderPass
{
  Core::Scope<Framebuffer> framebuffer{ nullptr };
  Core::Scope<Shader> shader{ nullptr };
  Core::Scope<GraphicsPipeline> pipeline{ nullptr };
  Core::Scope<Material> material{ nullptr };
};

struct SceneRendererCamera
{
  const Core::Camera& camera;
  glm::mat4 view_matrix;
  Core::f32 near;
  Core::f32 far;
  Core::f32 fov;
};

struct CommandKey
{
  const VertexBuffer* vertex_buffer{ nullptr };
  const IndexBuffer* index_buffer{ nullptr };
  Core::u32 submesh_index{ 0 };

  auto operator<=>(const CommandKey&) const = default;
};

class Renderer
{
public:
  struct Configuration
  {
    Core::u32 shadow_pass_size = 1024;
  };
  explicit Renderer(Configuration, const Window*);
  ~Renderer();

  auto begin_scene(Core::Scene&, const SceneRendererCamera&) -> void;
  auto end_scene() -> void;

  auto submit_static_mesh(const Graphics::VertexBuffer&,
                          const Graphics::IndexBuffer&,
                          const glm::mat4&,
                          const glm::vec4& = { 1, 1, 1, 1 }) -> void;

  auto get_output_image() const -> const Image*
  {
    return predepth_render_pass.framebuffer->get_colour_attachment(0);
  }

  auto on_resize(const Core::Extent&) -> void;

private:
  Core::Extent size{ 0, 0 };
  Core::Scope<CommandBuffer> command_buffer{ nullptr };

  RenderPass predepth_render_pass{};
  RenderPass shadow_render_pass{};

  auto construct_predepth_pass(const Window*) -> void;
  auto predepth_pass() -> void;

  auto construct_shadow_pass(const Window*, Core::u32) -> void;
  auto shadow_pass() -> void;

  auto flush_draw_lists() -> void;

  auto generate_and_update_descriptor_write_sets(const Shader*)
    -> VkDescriptorSet;
  auto get_buffer_info(const std::string_view) const
    -> const VkDescriptorBufferInfo*;

  // UBOs
  UniformBufferObject<RendererUBO> renderer_ubo{ "RendererUBO" };
  UniformBufferObject<ShadowUBO> shadow_ubo{ "ShadowUBO" };

  struct DrawCommand
  {
    const Graphics::VertexBuffer* vertex_buffer;
    const Graphics::IndexBuffer* index_buffer;
    Core::u32 submesh_index{ 0 };
    Core::u32 instance_count{ 0 };
  };

  std::unordered_map<CommandKey, DrawCommand> draw_commands;
  std::unordered_map<CommandKey, DrawCommand> shadow_draw_commands;

  std::vector<SubmeshTransformBuffer> transform_buffers;
  std::unordered_map<CommandKey, TransformMapData> mesh_transform_map;
};

}

namespace std {

inline auto
hash<Engine::Graphics::CommandKey>::operator()(
  const Engine::Graphics::CommandKey& key) const noexcept -> Engine::Core::usize
{
  return std::hash<const Engine::Graphics::VertexBuffer*>()(key.vertex_buffer) ^
         std::hash<const Engine::Graphics::IndexBuffer*>()(key.index_buffer) ^
         std::hash<Engine::Core::u32>()(key.submesh_index);
}

} // namespace std