#pragma once

#include "core/Forward.hpp"

#include "core/Types.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"
#include "graphics/Shader.hpp"

#include <glm/glm.hpp>

namespace Engine::Graphics {

struct SceneRendererCamera
{
  const Core::Camera& camera;
  glm::mat4 view_matrix;
  Core::f32 near;
  Core::f32 far;
  Core::f32 fov;
};

class Renderer
{
public:
  struct Configuration
  {
    Core::u32 shadow_pass_size = 1024;
  };
  explicit Renderer(Configuration, const Window*);

  auto begin_scene(Core::Scene&, const SceneRendererCamera&) -> void;
  auto end_scene() -> void;

private:
  Core::Extent size{ 0, 0 };

  Core::Scope<CommandBuffer> command_buffer{ nullptr };

  Core::Scope<Framebuffer> predepth_framebuffer{ nullptr };
  Core::Scope<Shader> predepth_shader{ nullptr };
  Core::Scope<GraphicsPipeline> predepth_pipeline{ nullptr };
  Core::Scope<Material> predepth_material{ nullptr };

  // TEMP
  Core::Scope<VertexBuffer> vertex_buffer{ nullptr };
  Core::Scope<IndexBuffer> index_buffer{ nullptr };

  struct ShadowPassParameters
  {
    Core::u32 size{ 0 };
  } shadow_pass_parameters;

  auto predepth_pass() -> void;
  auto flush_draw_lists() -> void;
};

}
