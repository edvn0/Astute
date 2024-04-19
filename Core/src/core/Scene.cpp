#include "pch/CorePCH.hpp"

#include "core/Scene.hpp"
#include "graphics/Vertex.hpp"

#include "graphics/Renderer.hpp"

namespace Engine::Core {

void
calculate_tangent_space_vectors(const glm::vec3& p1,
                                const glm::vec3& p2,
                                const glm::vec3& p3,
                                const glm::vec2& uv1,
                                const glm::vec2& uv2,
                                const glm::vec2& uv3,
                                glm::vec3& tangent,
                                glm::vec3& bitangent)
{
  glm::vec3 edge1 = p2 - p1;
  glm::vec3 edge2 = p3 - p1;
  glm::vec2 deltaUV1 = uv2 - uv1;
  glm::vec2 deltaUV2 = uv3 - uv1;

  float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

  tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
  tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
  tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
  tangent = glm::normalize(tangent);

  bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
  bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
  bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
  bitangent = glm::normalize(bitangent);
}

Scene::Scene(const std::string_view name_view)
  : name(name_view)
{

  std::vector<glm::vec3> positions = {
    { -1.0, 1.0, 1.0 },   { -1.0, -1.0, 1.0 }, { -1.0, 1.0, -1.0 },
    { -1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 },   { 1.0, -1.0, 1.0 },
    { 1.0, 1.0, -1.0 },   { 1.0, -1.0, -1.0 }
  };
  std::vector<glm::vec2> texCoords = {
    { 0.625, 0.5 },  { 0.375, 0.5 },  { 0.625, 0.75 }, { 0.375, 0.75 },
    { 0.875, 0.5 },  { 0.625, 0.25 }, { 0.125, 0.5 },  { 0.375, 0.25 },
    { 0.875, 0.75 }, { 0.625, 1.0 },  { 0.625, 0.0 },  { 0.375, 0.0 },
    { 0.375, 1.0 },  { 0.125, 0.75 }
  };
  std::vector<glm::vec3> normals = { { 0.0, 1.0, 0.0 },  { 0.0, 0.0, -1.0 },
                                     { 1.0, 0.0, 0.0 },  { 0.0, -1.0, 0.0 },
                                     { -1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 } };

  std::vector<Graphics::Vertex> vertices;
  std::vector<uint32_t> indices;
  std::unordered_map<Graphics::Vertex, uint32_t> uniqueVertices;

  // Faces for CCW winding
  std::vector<std::tuple<int, int, int>> faces = {
    { 5, 5, 1 },  { 1, 1, 1 },  { 3, 3, 1 },  { 3, 3, 2 },  { 4, 4, 2 },
    { 8, 13, 2 }, { 7, 11, 3 }, { 8, 12, 3 }, { 6, 8, 3 },  { 2, 2, 4 },
    { 6, 7, 4 },  { 8, 14, 4 }, { 1, 1, 5 },  { 2, 2, 5 },  { 4, 4, 5 },
    { 5, 6, 6 },  { 6, 8, 6 },  { 2, 2, 6 },  { 5, 5, 1 },  { 3, 3, 1 },
    { 7, 9, 1 },  { 3, 3, 2 },  { 8, 13, 2 }, { 7, 10, 2 }, { 7, 11, 3 },
    { 6, 8, 3 },  { 5, 6, 3 },  { 2, 2, 4 },  { 8, 14, 4 }, { 4, 4, 4 },
    { 1, 1, 5 },  { 4, 4, 5 },  { 3, 3, 5 },  { 5, 6, 6 },  { 2, 2, 6 },
    { 1, 1, 6 }
  };

  for (const auto& [zero, one, two] : faces) {
    Graphics::Vertex vertex;
    vertex.position = positions[zero - 1];
    vertex.uvs = texCoords[one - 1];
    vertex.normals = normals[two - 1];
    vertex.tangent = glm::vec3(0.0, 0.0, 0.0);
    vertex.bitangent = glm::vec3(0.0, 0.0, 0.0);
    vertex.colour = glm::vec4(1.0, 1.0, 1.0, 1.0);

    if (!uniqueVertices.contains(vertex)) {
      uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
      vertices.push_back(vertex);
    }
    indices.push_back(uniqueVertices[vertex]);
  }

  glm::vec3 tangent{};
  glm::vec3 bitangent{};
  calculate_tangent_space_vectors(vertices[0].position,
                                  vertices[1].position,
                                  vertices[2].position,
                                  vertices[0].uvs,
                                  vertices[1].uvs,
                                  vertices[2].uvs,
                                  tangent,
                                  bitangent);
  for (auto& vertex : vertices) { // Apply to all vertices of the face
    vertex.tangent = tangent;
    vertex.bitangent = bitangent;
  }

  auto entity = registry.create();
  auto& [vertex, index] = registry.emplace<SimpleMeshComponent>(entity);
  vertex = Core::make_ref<Graphics::VertexBuffer>(std::span{ vertices });
  index = Core::make_ref<Graphics::IndexBuffer>(std::span{ indices });
  auto& transform = registry.emplace<TransformComponent>(entity);
  transform.translation = { 0, 0, 0 };

  auto entity2 = registry.create();
  auto& [vertex2, index2] = registry.emplace<SimpleMeshComponent>(entity2);
  vertex2 = vertex;
  index2 = index;
  auto& transform2 = registry.emplace<TransformComponent>(entity2);
  transform2.translation = { 0, 5, 0 };
  // Floor is big!
  transform2.scale = { 10, 1, 10 };
}

auto
Scene::on_update_editor(f64 ts) -> void
{
  static f64 rotation = 0;
  rotation += ts;
  glm::vec3 begin{ 0 };
  begin.x = 10 * glm::cos(rotation);
  begin.y = -10;
  begin.z = 10 * glm::sin(rotation);

  light_environment.sun_position = begin;

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
  bool submitted_sun = false;
  for (auto&& [entity, mesh, transform] :
       registry.view<SimpleMeshComponent, TransformComponent>().each()) {
    renderer.submit_static_mesh(
      *mesh.vertex_buffer, *mesh.index_buffer, transform.compute());

    if (!submitted_sun) {
      submitted_sun = true;
      renderer.submit_static_mesh(
        *mesh.vertex_buffer,
        *mesh.index_buffer,
        glm::translate(glm::mat4{ 1 }, light_environment.sun_position));
    }
  }

  renderer.end_scene();
}

} // namespace Engine::Core
