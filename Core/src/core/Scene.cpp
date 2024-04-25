#include "pch/CorePCH.hpp"

#include "core/Random.hpp"
#include "core/Scene.hpp"
#include "graphics/Material.hpp"
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
  auto& [vertex, index, material, shader] =
    registry.emplace<SimpleMeshComponent>(entity);
  shader = Graphics::Shader::compile_graphics(
    "Assets/shaders/main_geometry.vert", "Assets/shaders/main_geometry.frag");
  vertex = Core::make_ref<Graphics::VertexBuffer>(std::span{ vertices });
  index = Core::make_ref<Graphics::IndexBuffer>(std::span{ indices });
  material = Core::make_ref<Graphics::Material>(
    Graphics::Material::Configuration{ shader.get() });
  material->set("normal_map",
                Graphics::TextureType::Normal,
                Graphics::Renderer::get_white_texture());

  auto& transform = registry.emplace<TransformComponent>(entity);
  transform.translation = { 0, 0, 0 };

  auto entity2 = registry.create();
  registry.emplace<SimpleMeshComponent>(
    entity2, vertex, index, material, shader);
  auto& transform2 = registry.emplace<TransformComponent>(entity2);
  transform2.translation = { 0, 5, 0 };
  // Floor is big!
  transform2.scale = { 10, 1, 10 };

  for (auto i = 0; i < 3; i++) {
    auto light = registry.create();
    registry.emplace<SimpleMeshComponent>(
      light, vertex, index, material, shader);
    auto& t = registry.emplace<TransformComponent>(light);
    auto& light_data = registry.emplace<PointLightComponent>(light);
    t.scale *= 0.1;
    t.translation = Random::random_in_rectangle(-30, 30);
    light_data.radiance = Random::random_colour();
  }

  for (auto i = 0; i < 3; i++) {
    auto light = registry.create();
    auto& t = registry.emplace<TransformComponent>(light);
    registry.emplace<SimpleMeshComponent>(
      light, vertex, index, material, shader);
    t.scale *= 0.1;

    t.translation = Random::random_in_rectangle(-5, 5);

    auto& light_data = registry.emplace<SpotLightComponent>(light);
    light_data.radiance = Random::random_colour();
  }
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

  light_environment.colour_and_intensity = { 0.6, 0.1, 0.7, 1 };
  light_environment.specular_colour_and_intensity = { 0.2, 0.9, 0, 1 };

  light_environment.spot_lights.clear();
  light_environment.point_lights.clear();

  static f32 time = 0;
  time += ts;

  [&]() {
    auto count = 0U;

    for (auto&& [entity, transform, point_light] :
         registry.view<TransformComponent, PointLightComponent>().each()) {
      auto& light = light_environment.point_lights.emplace_back();

      f32 radius = 5.0f;
      f32 speed = 1.0f;
      f32 phaseOffset = 0.5f;

      f32 angle = time * speed + phaseOffset * count;

      // Update position to move in a circle
      transform.translation.x = radius * cos(angle);
      transform.translation.z =
        radius * sin(angle); // Assuming circular motion in x-z plane

      light.pos = transform.translation;
      light.casts_shadows = point_light.casts_shadows;
      light.falloff = point_light.falloff;
      light.intensity = point_light.intensity;
      light.light_size = point_light.light_size;
      light.min_radius = point_light.min_radius;
      light.radiance = point_light.radiance;
      light.radius = point_light.radius;
      count++;
    }
  }();

  [&]() {
    auto count = 0;
    for (auto&& [entity, transform, spot_light] :
         registry.view<TransformComponent, SpotLightComponent>().each()) {
      auto& light = light_environment.spot_lights.emplace_back();
      f32 radius = 5.0f;
      f32 speed = 1.0f;
      f32 phaseOffset = 0.5f;

      f32 angle = time * speed + phaseOffset * static_cast<f32>(count);

      // Update position to move in a circle
      transform.translation.x = radius * cos(angle);
      transform.translation.y =
        radius * sin(angle); // Assuming circular motion in x-z plane

      light.pos = transform.translation;
      light.radiance = spot_light.radiance;
      light.intensity = spot_light.intensity;
      light.range = spot_light.range;
      light.angle = spot_light.angle;
      light.angle_attenuation = spot_light.angle_attenuation;
      light.casts_shadows = spot_light.casts_shadows;
      light.soft_shadows = spot_light.soft_shadows;
      light.falloff = spot_light.falloff;
      count++;
    }
  }();
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
    renderer.submit_static_mesh(*mesh.vertex_buffer,
                                *mesh.index_buffer,
                                *mesh.material,
                                transform.compute());

    if (!submitted_sun) {
      submitted_sun = true;
      renderer.submit_static_mesh(
        *mesh.vertex_buffer,
        *mesh.index_buffer,
        *mesh.material,
        glm::translate(glm::mat4{ 1 }, light_environment.sun_position));
    }
  }

  renderer.end_scene();
}

} // namespace Engine::Core
