#pragma once

#include "core/DataBuffer.hpp"
#include "core/Forward.hpp"
#include "core/Types.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"
#include "graphics/Mesh.hpp"
#include "graphics/RenderPass.hpp"
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

namespace Detail {
template<typename... Bases>
struct overload : Bases...
{
  using is_transparent = void;
  using Bases::operator()...;
};

struct CharPointerHash
{
  auto operator()(const char* ptr) const noexcept
  {
    return std::hash<std::string_view>{}(ptr);
  }
};

using transparent_string_hash = overload<std::hash<std::string>,
                                         std::hash<std::string_view>,
                                         CharPointerHash>;
}

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

struct SceneRendererCamera
{
  const Core::Camera& camera;
  glm::mat4 view_matrix{};
  Core::f32 near{};
  Core::f32 far{};
  Core::f32 fov{};
};

struct CommandKey
{
  const VertexBuffer* vertex_buffer{ nullptr };
  const IndexBuffer* index_buffer{ nullptr };
  const Material* material{ nullptr };
  Core::u32 submesh_index{ 0 };

  auto operator<=>(const CommandKey&) const = default;
};

enum class RendererTechnique : Core::u8
{
  Deferred,
  ForwardPlus
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

  auto submit_static_mesh(Core::Ref<StaticMesh>&, const glm::mat4&) -> void;
  auto submit_static_light(Core::Ref<StaticMesh>&, const glm::mat4&) -> void;

  auto get_output_image(Core::u32 attachment = 0) const -> const Image*
  {
    return render_passes.at("MainGeometry")
      ->get_framebuffer()
      ->get_colour_attachment(attachment)
      .get();
  }
  auto get_shadow_output_image() const -> const Image*
  {
    return render_passes.at("Shadow")
      ->get_framebuffer()
      ->get_depth_attachment()
      .get();
  }
  auto get_final_output() const -> const Image*
  {
    return render_passes.at("Lights")
      ->get_framebuffer()
      ->get_colour_attachment(0)
      .get();
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

  auto get_size() const -> const Core::Extent& { return size; }
  auto get_render_pass(const std::string& name) -> RenderPass&
  {
    return *render_passes.at(name);
  }
  auto get_render_pass(const std::string& name) const -> const RenderPass&
  {
    return *render_passes.at(name);
  }

  auto set_technique(RendererTechnique tech) -> void { technique = tech; }

  static auto get_thread_pool() -> ED::ThreadPool& { return *thread_pool; }

private:
  Core::Extent size{ 0, 0 };
  Core::Extent old_size{ 0, 0 };
  Core::Scope<CommandBuffer> command_buffer{ nullptr };
  Core::Scope<CommandBuffer> compute_command_buffer{ nullptr };

  std::unordered_map<std::string,
                     Core::Scope<RenderPass>,
                     Detail::transparent_string_hash,
                     std::equal_to<>>
    render_passes;

  auto flush_draw_lists() -> void;

  auto generate_and_update_descriptor_write_sets(Material&) -> VkDescriptorSet;

  // UBOs
  UniformBufferObject<RendererUBO> renderer_ubo{};
  UniformBufferObject<ShadowUBO> shadow_ubo{};
  UniformBufferObject<PointLightUBO> point_light_ubo{};
  UniformBufferObject<SpotLightUBO> spot_light_ubo{};
  UniformBufferObject<VisiblePointLightSSBO, GPUBufferType::Storage>
    visible_point_lights_ssbo{};
  UniformBufferObject<VisibleSpotLightSSBO, GPUBufferType::Storage>
    visible_spot_lights_ssbo{};
  UniformBufferObject<ScreenDataUBO> screen_data_ubo{};

  glm::uvec3 light_culling_work_groups{};

  struct DrawCommand
  {
    Core::Ref<StaticMesh> static_mesh{};
    Core::u32 submesh_index{ 0 };
    Core::u32 instance_count{ 0 };
  };

  std::unordered_map<CommandKey, DrawCommand> draw_commands;
  std::unordered_map<CommandKey, DrawCommand> shadow_draw_commands;
  std::unordered_map<CommandKey, DrawCommand> lights_draw_commands;

  std::vector<SubmeshTransformBuffer> transform_buffers;
  std::unordered_map<CommandKey, TransformMapData> mesh_transform_map;

  static inline Core::Ref<Image> white_texture;
  static inline Core::Ref<Image> black_texture;

  RendererTechnique technique{ RendererTechnique::Deferred };

  static inline Core::Scope<ED::ThreadPool> thread_pool{ nullptr };

  friend class RenderPass;
  friend class DeferredRenderPass;
  friend class MainGeometryRenderPass;
  friend class ShadowRenderPass;
  friend class PredepthRenderPass;
  friend class LightCullingRenderPass;
  friend class LightsRenderPass;
};

}

inline auto
std::hash<Engine::Graphics::CommandKey>::operator()(
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
