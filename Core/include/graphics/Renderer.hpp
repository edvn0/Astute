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
  std::array<glm::vec4, 3> transform_rows{};
};
struct TransformMapData
{
  std::vector<TransformVertexData> transforms;
  Core::u32 offset = 0;
};
struct SubmeshTransformBuffer
{
  Core::Scope<Graphics::VertexBuffer> transform_buffer{ nullptr };
  Core::Scope<Core::DataBuffer> data_buffer{ nullptr };
};

struct RenderPass
{
  Core::Scope<Framebuffer> framebuffer{ nullptr };
  Core::Scope<Shader> shader{ nullptr };
  Core::Scope<GraphicsPipeline> pipeline{ nullptr };
  Core::Scope<Material> material{ nullptr };

  auto destruct() -> void
  {
    framebuffer.reset();
    shader.reset();
    pipeline.reset();
    material.reset();
  }
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
  const Material* material{ nullptr };
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

  auto destruct() -> void;

  auto begin_scene(Core::Scene&, const SceneRendererCamera&) -> void;
  auto end_scene() -> void;

  auto submit_static_mesh(const Graphics::VertexBuffer&,
                          const Graphics::IndexBuffer&,
                          Graphics::Material&,
                          const glm::mat4&,
                          const glm::vec4& = { 1, 1, 1, 1 }) -> void;

  auto get_output_image(Core::u32 attachment = 0) const -> const Image*
  {
    return main_geometry_render_pass.framebuffer->get_colour_attachment(
      attachment);
  }
  auto get_shadow_output_image() const -> const Image*
  {
    return shadow_render_pass.framebuffer->get_depth_attachment().get();
  }
  auto get_final_output() const -> const Image*
  {
    return deferred_render_pass.framebuffer->get_colour_attachment(0);
  }

  auto on_resize(const Core::Extent&) -> void;

  static auto get_white_texture() -> const Core::Ref<Image>&
  {
    return white_texture;
  }
  static auto get_black_texture() -> const Core::Ref<Image>&
  {
    return black_texture;
  }

private:
  Core::Extent size{ 0, 0 };
  Core::Scope<CommandBuffer> command_buffer{ nullptr };

  RenderPass predepth_render_pass{};
  RenderPass main_geometry_render_pass{};
  RenderPass shadow_render_pass{};
  RenderPass deferred_render_pass{};

  auto construct_predepth_pass(const Window*) -> void;
  auto predepth_pass() -> void;

  auto construct_main_geometry_pass(const Window*, Core::Ref<Image>) -> void;
  auto main_geometry_pass() -> void;

  auto construct_shadow_pass(const Window*, Core::u32) -> void;
  auto shadow_pass() -> void;

  auto construct_deferred_pass(const Window*, const Framebuffer&) -> void;
  auto deferred_pass() -> void;

  auto flush_draw_lists() -> void;

  auto generate_and_update_descriptor_write_sets(const Shader*)
    -> VkDescriptorSet;
  auto generate_and_update_descriptor_write_sets(Material&) -> VkDescriptorSet;
  auto get_buffer_info(const std::string_view) const
    -> const VkDescriptorBufferInfo*;

  // UBOs
  UniformBufferObject<RendererUBO> renderer_ubo{};
  UniformBufferObject<ShadowUBO> shadow_ubo{};
  UniformBufferObject<PointLightUBO> point_light_ubo{};
  UniformBufferObject<SpotLightUBO> spot_light_ubo{};

  struct DrawCommand
  {
    const Graphics::VertexBuffer* vertex_buffer;
    const Graphics::IndexBuffer* index_buffer;
    Graphics::Material* material;
    Core::u32 submesh_index{ 0 };
    Core::u32 instance_count{ 0 };
  };

  std::unordered_map<CommandKey, DrawCommand> draw_commands;
  std::unordered_map<CommandKey, DrawCommand> shadow_draw_commands;

  std::vector<SubmeshTransformBuffer> transform_buffers;
  std::unordered_map<CommandKey, TransformMapData> mesh_transform_map;

  static inline Core::Ref<Image> white_texture;
  static inline Core::Ref<Image> black_texture;
};

}

namespace std {

inline auto
hash<Engine::Graphics::CommandKey>::operator()(
  const Engine::Graphics::CommandKey& key) const noexcept -> Engine::Core::usize
{
  static constexpr auto combine = []<class... T>(auto& seed,
                                                 const T&... values) {
    (...,
     (seed ^= std::hash<T>{}(values) + 0x9e3779b9 + (seed << 6) + (seed >> 2)));
    return seed;
  };
  std::size_t seed{ 0 };
  return combine(
    seed, key.vertex_buffer, key.index_buffer, key.material, key.submesh_index);
}

} // namespace std
