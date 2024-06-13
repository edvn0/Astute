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

  glm::vec4 min = { aabb.min.x, aabb.min.y, aabb.min.z, 1.0F };
  glm::vec4 max = { aabb.max.x, aabb.max.y, aabb.max.z, 1.0F };

  std::array<glm::vec4, 8> corners = {
    transform * glm::vec4{ aabb.min.x, aabb.min.y, aabb.max.z, 1.0F },
    transform * glm::vec4{ aabb.min.x, aabb.max.y, aabb.max.z, 1.0F },
    transform * glm::vec4{ aabb.max.x, aabb.max.y, aabb.max.z, 1.0F },
    transform * glm::vec4{ aabb.max.x, aabb.min.y, aabb.max.z, 1.0F },

    transform * glm::vec4{ aabb.min.x, aabb.min.y, aabb.min.z, 1.0F },
    transform * glm::vec4{ aabb.min.x, aabb.max.y, aabb.min.z, 1.0F },
    transform * glm::vec4{ aabb.max.x, aabb.max.y, aabb.min.z, 1.0F },
    transform * glm::vec4{ aabb.max.x, aabb.min.y, aabb.min.z, 1.0F }
  };

  for (Core::u32 i = 0; i < 4; i++) {
    const Line line{
      .from =
        LineVertex{
          .position = corners[i],
          .colour = colour,
        },
      .to =
        LineVertex{
          .position = corners[(i + 1) % 4],
          .colour = colour,
        },
    };
    submit_line(line);
  }

  for (Core::u32 i = 0; i < 4; i++) {
    const Line line{
      .from =
        LineVertex{
          .position = corners[i + 4],
          .colour = colour,
        },
      .to =
        LineVertex{
          .position = corners[((i + 1) % 4) + 4],
          .colour = colour,
        },
    };
    submit_line(line);
  }

  for (Core::u32 i = 0; i < 4; i++) {
    const Line line{
      .from =
        LineVertex{
          .position = corners[i],
          .colour = colour,
        },
      .to =
        LineVertex{
          .position = corners[i + 4],
          .colour = colour,
        },
    };
    submit_line(line);
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

  line_vertices->write(vertices.data(), submitted_line_indices);

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

  vkCmdBindPipeline(buffer.get_command_buffer(),
                    line_pipeline->get_bind_point(),
                    line_pipeline->get_pipeline());

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
