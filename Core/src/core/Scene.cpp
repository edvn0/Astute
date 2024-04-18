#include "pch/CorePCH.hpp"

#include "core/Scene.hpp"
#include "graphics/Vertex.hpp"

#include "graphics/Renderer.hpp"

namespace Engine::Core {

static constexpr std::array<const Graphics::Vertex, 8> cube_vertices{
  Graphics::Vertex{
    .position{ -0.5f, -0.5f, -0.5f },
    .uvs{ 0, 0 },
    .normals{ 0, 0, -1 },
  },
  Graphics::Vertex{
    .position{ 0.5f, -0.5f, -0.5f },
    .uvs{ 1, 0 },
    .normals{ 0, 0, -1 },
  },
  Graphics::Vertex{
    .position{ 0.5f, 0.5f, -0.5f },
    .uvs{ 1, 1 },
    .normals{ 0, 0, -1 },
  },
  Graphics::Vertex{
    .position{ -0.5f, 0.5f, -0.5f },
    .uvs{ 0, 1 },
    .normals{ 0, 0, -1 },
  },
  Graphics::Vertex{
    .position{ -0.5f, -0.5f, 0.5f },
    .uvs{ 0, 0 },
    .normals{ 0, 0, 1 },
  },
  Graphics::Vertex{
    .position{ 0.5f, -0.5f, 0.5f },
    .uvs{ 1, 0 },
    .normals{ 0, 0, 1 },
  },
  Graphics::Vertex{
    .position{ 0.5f, 0.5f, 0.5f },
    .uvs{ 1, 1 },
    .normals{ 0, 0, 1 },
  },
  Graphics::Vertex{
    .position{ -0.5f, 0.5f, 0.5f },
    .uvs{ 0, 1 },
    .normals{ 0, 0, 1 },
  },
};

static constexpr std::array<const Graphics::Vertex, 8>
  cube_vertices_counter_clockwise{
    Graphics::Vertex{
      .position{ -0.5f, -0.5f, -0.5f },
      .uvs{ 0, 0 },
      .normals{ 0, 0, -1 },
    },
    Graphics::Vertex{
      .position{ -0.5f, 0.5f, -0.5f },
      .uvs{ 0, 1 },
      .normals{ 0, 0, -1 },
    },
    Graphics::Vertex{
      .position{ 0.5f, 0.5f, -0.5f },
      .uvs{ 1, 1 },
      .normals{ 0, 0, -1 },
    },
    Graphics::Vertex{
      .position{ 0.5f, -0.5f, -0.5f },
      .uvs{ 1, 0 },
      .normals{ 0, 0, -1 },
    },
    Graphics::Vertex{
      .position{ -0.5f, -0.5f, 0.5f },
      .uvs{ 0, 0 },
      .normals{ 0, 0, 1 },
    },
    Graphics::Vertex{
      .position{ -0.5f, 0.5f, 0.5f },
      .uvs{ 0, 1 },
      .normals{ 0, 0, 1 },
    },
    Graphics::Vertex{
      .position{ 0.5f, 0.5f, 0.5f },
      .uvs{ 1, 1 },
      .normals{ 0, 0, 1 },
    },
    Graphics::Vertex{
      .position{ 0.5f, -0.5f, 0.5f },
      .uvs{ 1, 0 },
      .normals{ 0, 0, 1 },
    },
  };

static constexpr std::array<const Core::u32, 36> cube_indices{
  0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7,
  4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4,
};

Scene::Scene(const std::string_view name_view)
  : name(name_view)
{
  auto entity = registry.create();
  auto& [vertex, index] = registry.emplace<SimpleMeshComponent>(entity);
  vertex = Core::make_scope<Graphics::VertexBuffer>(
    std::span{ cube_vertices_counter_clockwise });
  index = Core::make_scope<Graphics::IndexBuffer>(std::span{ cube_indices });
  auto& transform = registry.emplace<TransformComponent>(entity);
  transform.translation = { 0, 0, 0 };
}

auto
Scene::on_update_editor(f64 ts) -> void
{
  light_environment.position = { -3, 3, 3 };
  light_environment.colour_and_intensity = { 1, 1, 1, 1 };
  light_environment.specular_colour_and_intensity = { 1, 1, 1, 1 };
}

auto
Scene::on_render_editor(Graphics::Renderer& renderer, const Camera& camera)
  -> void
{
  renderer.begin_scene(*this,
                       {
                         camera,
                         camera.get_view_matrix(),
                         camera.get_near_clip(),
                         camera.get_far_clip(),
                         camera.get_fov(),
                       });

  for (auto&& [entity, mesh, transform] :
       registry.view<SimpleMeshComponent, TransformComponent>().each()) {
    renderer.submit_static_mesh(
      *mesh.vertex_buffer, *mesh.index_buffer, transform.compute());
  }

  renderer.end_scene();
}

} // namespace Engine::Core
