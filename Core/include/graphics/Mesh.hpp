#pragma once

#include "core/Types.hpp"
#include "graphics/Material.hpp"
#include "graphics/Vertex.hpp"

#include <vector>

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
class Importer;
}

namespace Engine::Graphics {

struct AABB
{
  glm::vec3 min, max;

  constexpr AABB()
    : min(0.0f)
    , max(0.0f)
  {
  }

  constexpr AABB(const glm::vec3& in_min, const glm::vec3& in_max)
    : min(in_min)
    , max(in_max)
  {
  }

  auto update_min_max(const glm::vec3& new_position) -> void
  {
    min.x = glm::min(new_position.x, min.x);
    min.y = glm::min(new_position.y, min.y);
    min.z = glm::min(new_position.z, min.z);

    max.x = glm::max(new_position.x, max.x);
    max.y = glm::max(new_position.y, max.y);
    max.z = glm::max(new_position.z, max.z);
  }
};

struct Index
{
  Core::u32 V1, V2, V3;
};

static_assert(sizeof(Index) == 3 * sizeof(Core::u32));

struct Triangle
{
  Vertex V0{};
  Vertex V1{};
  Vertex V2{};

  Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
    : V0(v0)
    , V1(v1)
    , V2(v2)
  {
  }
};

class Submesh
{
public:
  Core::u32 base_vertex;
  Core::u32 base_index;
  Core::u32 material_index;
  Core::u32 index_count;
  Core::u32 vertex_count;

  glm::mat4 transform{ 1.0f };
  glm::mat4 local_transform{ 1.0f };
  AABB bounding_box;

  std::string node_name;
  std::string mesh_name;
  // bool IsRigged = false;
};

class MeshAsset
{
public:
  explicit MeshAsset(const std::string&);
  ~MeshAsset() = default;

  auto get_submeshes() -> auto& { return submeshes; }
  auto get_submeshes() const -> const auto& { return submeshes; }

  auto get_vertices() const -> const auto& { return vertices; }
  auto get_indices() const -> const auto& { return indices; }

  auto get_materials() -> auto& { return materials; }
  auto get_materials() const -> const auto& { return materials; }
  auto get_file_path() const -> const std::string_view { return file_path; }

  auto get_triangle_cache(Core::u32 index) const -> const auto&
  {
    return triangle_cache.at(index);
  }

  auto get_vertex_buffer() const -> const auto& { return *vertex_buffer; }
  auto get_index_buffer() const -> const auto& { return *index_buffer; }

  const AABB& get_bounding_box() const { return bounding_box; }

private:
  void traverse_nodes(aiNode* node,
                      const glm::mat4& parentTransform = glm::mat4(1.0f),
                      Core::u32 level = 0);

private:
  std::vector<Submesh> submeshes;

  Core::Scope<Assimp::Importer> importer;

  Core::Scope<VertexBuffer> vertex_buffer;
  Core::Scope<IndexBuffer> index_buffer;
  Core::Scope<Shader> deferred_pbr_shader;

  std::vector<Vertex> vertices;
  std::vector<Index> indices;
  std::unordered_map<aiNode*, std::vector<Core::u32>> ai_node_map;
  const aiScene* ai_scene;

  std::vector<Core::Scope<Material>> materials;
  std::unordered_map<Core::u32, std::vector<Triangle>> triangle_cache;

  AABB bounding_box;

  std::string file_path;

  friend class Core::Scene;
  friend class Renderer;
};

class Mesh
{
public:
  explicit Mesh(Core::Ref<MeshAsset> mesh_asset);
  Mesh(Core::Ref<MeshAsset> mesh_asset,
       const std::vector<Core::u32>& submeshes);
  Mesh(const Core::Ref<Mesh>& other);
  ~Mesh();

  auto get_submeshes() -> auto& { return submeshes; }
  auto get_submeshes() const -> const auto& { return submeshes; }

  void set_submeshes(const std::vector<Core::u32>& submeshes);

  auto get_mesh_asset() -> auto& { return mesh_asset; }
  auto get_mesh_asset() const -> const auto& { return mesh_asset; }
  void set_mesh_asset(Core::Ref<MeshAsset> asset) { mesh_asset = asset; }

  auto get_materials() const -> const auto&
  {
    return mesh_asset->get_materials();
  }

private:
  Core::Ref<MeshAsset> mesh_asset;
  std::vector<Core::u32> submeshes;

  friend class Core::Scene;
  friend class Renderer;
};

class StaticMesh
{
public:
  explicit StaticMesh(Core::Ref<MeshAsset>);
  StaticMesh(Core::Ref<MeshAsset>, const std::vector<Core::u32>&);
  StaticMesh(const Core::Ref<StaticMesh>&);
  ~StaticMesh() = default;

  auto get_submeshes() -> auto& { return submeshes; }
  auto get_submeshes() const -> const auto& { return submeshes; }

  void set_submeshes(const std::vector<Core::u32>&);

  auto get_mesh_asset() -> auto& { return mesh_asset; }
  auto get_mesh_asset() const -> const auto& { return mesh_asset; }
  void set_mesh_asset(Core::Ref<MeshAsset> asset) { mesh_asset = asset; }

  auto get_materials() const -> const auto&
  {
    return mesh_asset->get_materials();
  }
  auto get_materials() -> auto& { return mesh_asset->get_materials(); }

private:
  Core::Ref<MeshAsset> mesh_asset;
  std::vector<Core::u32> submeshes;

  friend class Scene;
  friend class Renderer;
};

} // namespace Engine::Graphics
