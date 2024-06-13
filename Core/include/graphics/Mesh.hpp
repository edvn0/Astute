#pragma once

#include "core/AABB.hpp"
#include "core/Types.hpp"
#include "graphics/Material.hpp"
#include "graphics/Vertex.hpp"

#include "thread_pool/CommandBufferDispatcher.hpp"

#include <vector>

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
class Importer;
}

namespace Engine::Graphics {

enum class TextureType : Core::u8
{
  Albedo,
  Normal,
  Specular,
  Roughness,
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

  glm::mat4 transform{ 1.0F };
  glm::mat4 local_transform{ 1.0F };
  Core::AABB bounding_box;

  std::string node_name;
  std::string mesh_name;
};

class MeshAsset
{
public:
  explicit MeshAsset(const std::string&);
  ~MeshAsset();

  [[nodiscard]] auto get_submeshes() -> auto& { return submeshes; }
  [[nodiscard]] auto get_submeshes() const -> const auto& { return submeshes; }

  [[nodiscard]] auto get_vertices() const -> const auto& { return vertices; }
  [[nodiscard]] auto get_indices() const -> const auto& { return indices; }

  [[nodiscard]] auto get_materials() -> auto& { return materials; }
  [[nodiscard]] auto get_materials() const -> const auto& { return materials; }
  [[nodiscard]] auto get_file_path() const -> std::string_view
  {
    return file_path;
  }

  [[nodiscard]] auto get_triangle_cache(Core::u32 index) const -> const auto&
  {
    return triangle_cache.at(index);
  }

  [[nodiscard]] auto get_vertex_buffer() const -> const auto&
  {
    return *vertex_buffer;
  }
  [[nodiscard]] auto get_index_buffer() const -> const auto&
  {
    return *index_buffer;
  }

  [[nodiscard]] auto get_bounding_box() const -> const Core::AABB&
  {
    return bounding_box;
  }

private:
  void traverse_nodes(aiNode*,
                      const glm::mat4& = glm::mat4(1.0F),
                      Core::u32 = 0);

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

  Core::AABB bounding_box;

  std::string file_path;

  Core::Scope<CommandBuffer> command_buffer;
  ED::CommandBufferDispatcher dispatcher;
  std::mutex mutex;
  std::unordered_map<Core::u32,
                     std::unordered_map<TextureType, Core::Ref<Image>>>
    output_images;

  friend class Core::Scene;
  friend class Renderer;
};

class Mesh
{
public:
  explicit Mesh(Core::Ref<MeshAsset> mesh_asset);
  Mesh(Core::Ref<MeshAsset> mesh_asset,
       const std::vector<Core::u32>& submeshes);
  explicit Mesh(const Core::Ref<Mesh>& other);
  ~Mesh();

  [[nodiscard]] auto get_submeshes() -> auto& { return submeshes; }
  [[nodiscard]] auto get_submeshes() const -> const auto& { return submeshes; }

  void set_submeshes(const std::vector<Core::u32>& submeshes);

  [[nodiscard]] auto get_mesh_asset() -> auto& { return mesh_asset; }
  [[nodiscard]] auto get_mesh_asset() const -> const auto&
  {
    return mesh_asset;
  }
  void set_mesh_asset(Core::Ref<MeshAsset> asset)
  {
    mesh_asset = std::move(asset);
  }

  [[nodiscard]] auto get_materials() const -> const auto&
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
  explicit StaticMesh(const Core::Ref<StaticMesh>&);
  ~StaticMesh() = default;

  auto get_submeshes() -> auto& { return submeshes; }
  [[nodiscard]] auto get_submeshes() const -> const auto& { return submeshes; }

  void set_submeshes(const std::vector<Core::u32>&);

  auto get_mesh_asset() -> auto& { return mesh_asset; }
  [[nodiscard]] auto get_mesh_asset() const -> const auto&
  {
    return mesh_asset;
  }
  void set_mesh_asset(Core::Ref<MeshAsset> asset)
  {
    mesh_asset = std::move(asset);
  }

  [[nodiscard]] auto get_materials() const -> const auto&
  {
    return mesh_asset->get_materials();
  }
  auto get_materials() -> auto& { return mesh_asset->get_materials(); }

  static auto construct(std::string_view) -> Core::Ref<StaticMesh>;

private:
  Core::Ref<MeshAsset> mesh_asset;
  std::vector<Core::u32> submeshes;

  friend class Scene;
  friend class Renderer;
};

} // namespace Engine::Graphics
