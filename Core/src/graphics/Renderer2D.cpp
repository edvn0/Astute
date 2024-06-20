#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"
#include "graphics/Renderer2D.hpp"

#include <ranges>
#include <span>

namespace Engine::Graphics {

Renderer2D::Renderer2D(Renderer& ren, Core::u32 object_count)
  : renderer(&ren)
{
  vertices.resize(static_cast<Core::usize>(object_count));
  std::vector<Core::u32> indices(object_count);
  for (const auto i : std::views::iota(0U, object_count)) {
    indices.at(i) = i;
  }

  line_vertices = Core::make_scope<VertexBuffer>(std::span(vertices));
  line_indices = Core::make_scope<IndexBuffer>(std::span(indices));
  line_shader = Shader::compile_graphics_scoped("Assets/shaders/line.vert",
                                                "Assets/shaders/line.frag");
  line_material = Core::make_scope<Material>(Material::Configuration{
    .shader = line_shader.get(),
  });
  GraphicsPipeline::Configuration config{
    .framebuffer =
      renderer->get_render_pass("MainGeometry").get_framebuffer().get(),
    .shader = line_shader.get(),
    .face_mode = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .topology = Topology::LineList,
    .override_vertex_attributes = { {
      {
        0,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        0,
      },
      {
        1,
        0,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        sizeof(float) * 3,
      },
    } },
    .override_instance_attributes = { {}, },
  };
  line_pipeline = Core::make_scope<GraphicsPipeline>(config);
}

auto
Renderer2D::submit_line(const Line& line) -> void
{
  vertices.emplace_back(line.from.position, line.from.colour);
  vertices.emplace_back(line.to.position, line.to.colour);
  submitted_line_indices += 2;
}

auto
Renderer2D::submit_aabb(const Core::AABB& aabb,
                        const glm::mat4& transform,
                        const glm::vec4& colour) -> void
{
  ASTUTE_PROFILE_FUNCTION();
  const auto min_aabb = aabb.min;
  const auto max_aabb = aabb.max;
  const std::array corners = {
    transform * glm::vec4{ min_aabb.x, min_aabb.y, max_aabb.z, 1.0F },
    transform * glm::vec4{ min_aabb.x, max_aabb.y, max_aabb.z, 1.0F },
    transform * glm::vec4{ max_aabb.x, max_aabb.y, max_aabb.z, 1.0F },
    transform * glm::vec4{ max_aabb.x, min_aabb.y, max_aabb.z, 1.0F },

    transform * glm::vec4{ min_aabb.x, min_aabb.y, min_aabb.z, 1.0F },
    transform * glm::vec4{ min_aabb.x, max_aabb.y, min_aabb.z, 1.0F },
    transform * glm::vec4{ max_aabb.x, max_aabb.y, min_aabb.z, 1.0F },
    transform * glm::vec4{ max_aabb.x, min_aabb.y, min_aabb.z, 1.0F }
  };

  static constexpr std::array<Core::u32, 24> indices = {
    0, 1, 1, 2, 2, 3, 3, 0, // Top face
    4, 5, 5, 6, 6, 7, 7, 4, // Bottom face
    0, 4, 1, 5, 2, 6, 3, 7  // Sides
  };

  for (Core::u32 i = 0; i < indices.size(); i += 2) {
    LineVertex from{
      .position = corners[indices[i]],
      .colour = colour,
    };
    LineVertex to{
      .position = corners[indices[i + 1]],
      .colour = colour,
    };
    submit_line({
      .from = from,
      .to = to,
    });
  }
}

auto
Renderer2D::flush(CommandBuffer& buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();
  // Lets assume main geometry is started
  const auto span = std::span(vertices);
  if (span.size_bytes() >= line_vertices->size()) {
    line_vertices = Core::make_scope<VertexBuffer>(span);

    std::vector<Core::u32> new_indices;
    new_indices.resize(submitted_line_indices);
    for (const auto i : std::views::iota(0U, submitted_line_indices)) {
      new_indices.at(i) = i;
    }
    line_indices = Core::make_scope<IndexBuffer>(std::span(new_indices));
  }
  vkCmdBindPipeline(buffer.get_command_buffer(),
                    line_pipeline->get_bind_point(),
                    line_pipeline->get_pipeline());

  line_vertices->write(vertices.data(),
                       submitted_line_indices * sizeof(LineVertex));

  static constexpr auto offsets = std::array<VkDeviceSize, 1>{ 0 };
  std::array buffers{ line_vertices->get_buffer() };
  vkCmdBindVertexBuffers(
    buffer.get_command_buffer(), 0, 1, buffers.data(), offsets.data());

  auto* renderer_desc_set =
    renderer->generate_and_update_descriptor_write_sets(*line_material);

  std::array desc_sets{ renderer_desc_set };
  vkCmdBindDescriptorSets(buffer.get_command_buffer(),
                          line_pipeline->get_bind_point(),
                          line_pipeline->get_layout(),
                          0,
                          static_cast<Core::u32>(desc_sets.size()),
                          desc_sets.data(),
                          0,
                          nullptr);

  vkCmdBindIndexBuffer(buffer.get_command_buffer(),
                       line_indices->get_buffer(),
                       0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdSetLineWidth(buffer.get_command_buffer(), 5.0F);

  vkCmdDrawIndexed(buffer.get_command_buffer(),
                   static_cast<Core::u32>(submitted_line_indices),
                   1,
                   0,
                   0,
                   0);

  const auto size = vertices.size();
  vertices.clear();
  vertices.reserve(size);
  submitted_line_indices = 0;
}

} // namespace Engine::Graphics
