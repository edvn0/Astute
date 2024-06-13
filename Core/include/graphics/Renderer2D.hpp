#pragma once

#include "graphics/Forward.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"
#include "graphics/Shader.hpp"

#include "core/AABB.hpp"

#include <glm/glm.hpp>

namespace Engine::Graphics {

struct LineVertex
{
  const glm::vec3 position{};
  const glm::vec4 colour{};
};

struct Line
{
  const LineVertex from{};
  const LineVertex to{};
};

class Renderer2D
{
public:
  explicit Renderer2D(Renderer&, Core::u32);
  ~Renderer2D() = default;

  auto submit_line(const Line&) -> void;
  auto submit_aabb(const Core::AABB&, const glm::mat4&, const glm::vec4&)
    -> void;

  auto flush(CommandBuffer&) -> void;

private:
  Renderer* renderer;
  std::vector<LineVertex> vertices;

  Core::Scope<VertexBuffer> line_vertices;
  Core::Scope<IndexBuffer> line_indices;
  Core::Scope<GraphicsPipeline> line_pipeline;
  Core::Scope<Material> line_material;
  Core::Scope<Shader> line_shader;
  Core::u32 submitted_line_indices{ 0 };
};

} // namespace Engine::Graphics
