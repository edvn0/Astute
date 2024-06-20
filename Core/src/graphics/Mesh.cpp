#include "graphics/CommandBuffer.hpp"
#include "pch/CorePCH.hpp"

#include "graphics/Mesh.hpp"

#include "graphics/Renderer.hpp"
#include "logging/Logger.hpp"
#include "thread_pool/CommandBufferDispatcher.hpp"

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <utility>
namespace Engine::Graphics {

static constexpr auto maybe_load_embedded_texture =
  [](auto& dispatcher,
     auto index,
     TextureType T,
     const std::string& name,
     const aiTexture* embedded_texture,
     auto& outputs,
     auto& mutex,
     auto& cache) -> void {
  if (embedded_texture == nullptr) {
    throw std::runtime_error("No texture");
  }

  Core::DataBuffer buffer{ embedded_texture->mWidth *
                           embedded_texture->mHeight * 4 };
  buffer.write(embedded_texture->pcData,
               embedded_texture->mWidth * embedded_texture->mHeight * 4);

  std::unique_lock lock(mutex);
  auto& back = cache.emplace_back();
  back = Core::make_ref<StagingBuffer>(std::move(buffer));
  lock.unlock();

  const auto width = embedded_texture->mWidth;
  const auto height = embedded_texture->mHeight;

  dispatcher.dispatch(
    [=, staging = back, &images = outputs, &m = mutex](auto* cmd) {
      std::lock_guard lock(m);
      images[index][T] = Image::load_from_memory(cmd,
                                                 width,
                                                 height,
                                                 staging,
                                                 {
                                                   .path = name,
                                                   .use_mips = true,
                                                 });
    });
};

static constexpr auto load_texture_from_file =
  [](auto& dispatcher,
     auto index,
     TextureType T,
     const std::string& base_path,
     const std::string& texture_path,
     auto& outputs,
     auto& mutex,
     auto& cache) -> void {
  std::filesystem::path path = base_path;
  auto parent_path = path.parent_path();
  parent_path /= texture_path;
  std::string real_path = parent_path.string();

  Core::u32 w{};
  Core::u32 h{};

  std::unique_lock lock(mutex);
  auto& back = cache.emplace_back();
  back = Image::load_from_file_into_staging(real_path, &w, &h);
  lock.unlock();

  dispatcher.dispatch([=, c = back, &images = outputs, &m = mutex](auto* cmd) {
    std::lock_guard lock(m);
    images[index][T] = Image::load_from_memory(cmd,
                                               w,
                                               h,
                                               c,
                                               {
                                                 .path = real_path,
                                                 .use_mips = true,
                                               });
  });
};

struct AssimpLogStream : public Assimp::LogStream
{
  static void initialize()
  {
    if (Assimp::DefaultLogger::isNullLogger()) {
      Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
      Assimp::DefaultLogger::get()->attachStream(
        new AssimpLogStream,
        Assimp::Logger::Debugging | Assimp::Logger::Info |
          Assimp::Logger::Warn | Assimp::Logger::Err);
    }
  }

  void write(const char* message) override
  {
    std::string msg(message);
    if (!msg.empty() && msg[msg.length() - 1] == '\n') {
      msg.erase(msg.length() - 1);
    }
    if (strncmp(message, "Debug", 5) == 0) {
      trace("Assimp: {}", msg);
    } else if (strncmp(message, "Info", 4) == 0) {
      trace("Assimp: {}", msg);
    } else if (strncmp(message, "Warn", 4) == 0) {
      warn("Assimp: {}", msg);
    } else {
      error("Assimp: {}", msg);
    }
  }
};

static constexpr Core::u32 mesh_import_flags =
  aiProcess_CalcTangentSpace | // Calculate tangents and bitangents if you use
                               // normal mapping
  aiProcess_JoinIdenticalVertices | // Join identical vertices to optimize mesh
  aiProcess_Triangulate |           // Ensure all faces are triangles
  aiProcess_GenSmoothNormals |      // Generate smooth normals if not present
  aiProcess_SplitLargeMeshes | // Split large meshes into smaller sub-meshes
  aiProcess_LimitBoneWeights | // Limit bone weights to 4 per vertex (typical
                               // for GPU skinning)
  aiProcess_ValidateDataStructure | // Ensure the data structure is valid
  aiProcess_ImproveCacheLocality |  // Improve the cache locality of the vertex
                                    // buffer
  aiProcess_RemoveRedundantMaterials | // Remove redundant materials
  aiProcess_FindDegenerates | // Convert degenerate primitives to proper
                              // lines/points
  aiProcess_FindInvalidData | // Detect and fix invalid data, such as
                              // zero-length normals
  aiProcess_GenUVCoords | // Convert non-UV mappings to proper UV coordinates
  aiProcess_TransformUVCoords | // Pre-transform UV coordinates
  aiProcess_OptimizeMeshes |    // Optimize the mesh for fewer draw calls
  aiProcess_OptimizeGraph |     // Optimize the scene graph
  aiProcess_FlipUVs |          // Flip UV coordinates for DirectX-style textures
  aiProcess_FlipWindingOrder | // Ensure the face winding order is clockwise
  aiProcess_Debone;            // Remove bones if they affect vertices minimally

namespace Utils {

auto
mat4_from_assimp_matrix4(const aiMatrix4x4& matrix) -> glm::mat4
{
  glm::mat4 result;
  // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
  result[0][0] = matrix.a1;
  result[1][0] = matrix.a2;
  result[2][0] = matrix.a3;
  result[3][0] = matrix.a4;
  result[0][1] = matrix.b1;
  result[1][1] = matrix.b2;
  result[2][1] = matrix.b3;
  result[3][1] = matrix.b4;
  result[0][2] = matrix.c1;
  result[1][2] = matrix.c2;
  result[2][2] = matrix.c3;
  result[3][2] = matrix.c4;
  result[0][3] = matrix.d1;
  result[1][3] = matrix.d2;
  result[2][3] = matrix.d3;
  result[3][3] = matrix.d4;
  return result;
}
}

MeshAsset::MeshAsset(const std::string& file_name)
  : command_buffer{
    Core::make_scope<CommandBuffer>(CommandBuffer::Properties{
      .queue_type = QueueType::Graphics,
      .primary = true,
      .image_count = 1,
    }),
  }, dispatcher(*command_buffer)
{
  deferred_pbr_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/main_geometry.vert", "Assets/shaders/main_geometry.frag");

  AssimpLogStream::initialize();

  info("Loading mesh: {0}", file_name.c_str());

  importer = Core::make_scope<Assimp::Importer>();
  importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 100.0F);

  const aiScene* scene = importer->ReadFile(file_name, mesh_import_flags);
  if (scene == nullptr) {
    error("Failed to load mesh file: {0}", file_name);
    return;
  }

  ai_scene = scene;

  if (!scene->HasMeshes()) {
    return;
  }
  static constexpr auto flt_max = std::numeric_limits<float>::max();

  Core::u32 vertex_count = 0;
  Core::u32 index_count = 0;

  bounding_box.min = { flt_max, flt_max, flt_max };
  bounding_box.max = { -flt_max, -flt_max, -flt_max };

  submeshes.reserve(scene->mNumMeshes);
  for (unsigned m = 0; m < scene->mNumMeshes; m++) {
    aiMesh* mesh = scene->mMeshes[m];

    Submesh& submesh = submeshes.emplace_back();
    submesh.base_vertex = vertex_count;
    submesh.base_index = index_count;
    submesh.material_index = mesh->mMaterialIndex;
    submesh.vertex_count = mesh->mNumVertices;
    submesh.index_count = mesh->mNumFaces * 3;
    submesh.mesh_name = mesh->mName.C_Str();

    vertex_count += mesh->mNumVertices;
    index_count += submesh.index_count;

    // Vertices
    auto& aabb = submesh.bounding_box;
    aabb.min = {
      flt_max,
      flt_max,
      flt_max,
    };
    aabb.max = {
      -flt_max,
      -flt_max,
      -flt_max,
    };

    const auto count = mesh->mNumVertices;

    const auto vertices_span = std::span{ mesh->mVertices, count };
    const auto normals_span = std::span{ mesh->mNormals, count };
    const auto tangents_span = std::span{ mesh->mTangents, count };
    const auto bitangents_span = std::span{ mesh->mBitangents, count };
    const auto uvs_span = std::span{ mesh->mTextureCoords, count };
    const auto index_span = std::span{ mesh->mFaces, mesh->mNumFaces };

    const auto has_normals = mesh->HasNormals();
    const auto has_tangents = mesh->HasTangentsAndBitangents();
    const auto has_uvs = mesh->HasTextureCoords(0);

    for (auto i = 0U; i < count; i++) {
      Vertex vertex{};
      vertex.position = {
        vertices_span[i].x,
        vertices_span[i].y,
        vertices_span[i].z,
      };
      vertex.normals = {
        has_normals ? normals_span[i].x : 0.0F,
        has_normals ? normals_span[i].y : 0.0F,
        has_normals ? normals_span[i].z : 0.0F,
      };

      aabb.update_min_max(vertex.position);

      if (has_tangents) {
        vertex.tangent = {
          tangents_span[i].x,
          tangents_span[i].y,
          tangents_span[i].z,
        };
        vertex.bitangent = {
          bitangents_span[i].x,
          bitangents_span[i].y,
          bitangents_span[i].z,
        };
      }

      if (has_uvs) {
        vertex.uvs = {
          uvs_span[0][i].x,
          uvs_span[0][i].y,
        };
      }

      vertices.push_back(vertex);
    }

    // Indices
    for (auto i = 0U; i < mesh->mNumFaces; i++) {
      Index index = {
        .V1 = index_span[i].mIndices[0],
        .V2 = index_span[i].mIndices[1],
        .V3 = index_span[i].mIndices[2],
      };
      indices.push_back(index);

      triangle_cache[m].emplace_back(vertices[index.V1 + submesh.base_vertex],
                                     vertices[index.V2 + submesh.base_vertex],
                                     vertices[index.V3 + submesh.base_vertex]);
    }
  }

  traverse_nodes(scene->mRootNode);

  for (const auto& submesh : submeshes) {
    Core::AABB transformed_submesh_aabb = submesh.bounding_box;
    glm::vec3 min = glm::vec3(submesh.transform *
                              glm::vec4(transformed_submesh_aabb.min, 1.0F));
    glm::vec3 max = glm::vec3(submesh.transform *
                              glm::vec4(transformed_submesh_aabb.max, 1.0F));

    bounding_box.update_min_max(min);
    bounding_box.update_min_max(max);
  }

  if (!scene->HasMaterials()) {
    return;
  }

  std::vector<Core::Ref<StagingBuffer>> cache;
  cache.reserve(150);

  std::span scene_mats{ scene->mMaterials, scene->mNumMaterials };
  materials.resize(scene_mats.size());
  const auto& white_texture = Renderer::get_white_texture();
  auto i = 0ULL;
  for (const auto& ai_material : scene_mats) {
    materials.at(i) = Core::make_scope<Material>(Material::Configuration{
      .shader = deferred_pbr_shader.get(),
    });
    aiString ai_tex_path;

    float shininess{};
    float metalness{};
    if (ai_material->Get(AI_MATKEY_SHININESS, shininess) != aiReturn_SUCCESS) {
      shininess = 80.0F;
    }

    if (ai_material->Get(AI_MATKEY_REFLECTIVITY, metalness) !=
        aiReturn_SUCCESS) {
      metalness = 0.0F;
    }
    auto roughness = 1.0F - glm::sqrt(shininess / 100.0F);

    materials.at(i)->set("mat_pc.albedo_colour", glm::vec3(1.0F));
    materials.at(i)->set("mat_pc.emission", 1.0F);

    materials.at(i)->set("mat_pc.use_normal_map", false);

    materials.at(i)->set("mat_pc.roughness", roughness);

    const auto casted_index = static_cast<Core::u32>(i);

    bool has_albedo_map =
      ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &ai_tex_path) ==
      AI_SUCCESS;
    if (has_albedo_map) {
      if (const auto* embedded_texture =
            scene->GetEmbeddedTexture(ai_tex_path.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    casted_index,
                                    TextureType::Albedo,
                                    std::string{ ai_tex_path.C_Str() },
                                    embedded_texture,
                                    output_images,
                                    mutex,
                                    cache);
      } else {
        load_texture_from_file(dispatcher,
                               casted_index,
                               TextureType::Albedo,
                               file_name,
                               ai_tex_path.C_Str(),
                               output_images,
                               mutex,
                               cache);
      }
    }

    // Normal maps
    bool has_normal_map =
      ai_material->GetTexture(aiTextureType_NORMALS, 0, &ai_tex_path) ==
      AI_SUCCESS;
    if (has_normal_map) {
      if (const auto* embedded_texture =
            scene->GetEmbeddedTexture(ai_tex_path.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    casted_index,
                                    TextureType::Normal,
                                    std::string{ ai_tex_path.C_Str() },
                                    embedded_texture,
                                    output_images,
                                    mutex,
                                    cache);
      } else {
        load_texture_from_file(dispatcher,
                               casted_index,
                               TextureType::Normal,
                               file_name,
                               ai_tex_path.C_Str(),
                               output_images,
                               mutex,
                               cache);
      }
    }

    // Specular maps
    bool has_specular_map =
      ai_material->GetTexture(aiTextureType_SPECULAR, 0, &ai_tex_path) ==
      AI_SUCCESS;
    if (has_specular_map) {
      if (const auto* embedded_texture =
            scene->GetEmbeddedTexture(ai_tex_path.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    casted_index,
                                    TextureType::Specular,
                                    std::string{ ai_tex_path.C_Str() },
                                    embedded_texture,
                                    output_images,
                                    mutex,
                                    cache);
      } else {
        load_texture_from_file(dispatcher,
                               casted_index,
                               TextureType::Specular,
                               file_name,
                               ai_tex_path.C_Str(),

                               output_images,
                               mutex,
                               cache);
      }
    }

    // Roughness map
    bool has_roughness_map =
      ai_material->GetTexture(aiTextureType_SHININESS, 0, &ai_tex_path) ==
      AI_SUCCESS;

    aiString combined_roughness_metallic_file;
    ai_material->GetTexture(
      aiTextureType_UNKNOWN, 0, &combined_roughness_metallic_file);

    bool prefer_combined = false;
    if (combined_roughness_metallic_file.length > 0) {
      prefer_combined = true;
      has_roughness_map = true;
    }

    if (has_roughness_map) {
      if (const auto* embedded_texture = scene->GetEmbeddedTexture(
            prefer_combined ? combined_roughness_metallic_file.C_Str()
                            : ai_tex_path.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    casted_index,
                                    TextureType::Roughness,
                                    std::string{ ai_tex_path.C_Str() },
                                    embedded_texture,
                                    output_images,
                                    mutex,
                                    cache);
      } else {
        load_texture_from_file(dispatcher,
                               casted_index,
                               TextureType::Roughness,
                               file_name,
                               prefer_combined
                                 ? combined_roughness_metallic_file.C_Str()
                                 : ai_tex_path.C_Str(),
                               output_images,
                               mutex,
                               cache);
      }
    }

    i++;
  }

  dispatcher.execute();

  vertex_buffer = Core::make_scope<VertexBuffer>(std::span{ vertices });
  index_buffer = Core::make_scope<IndexBuffer>(indices.data(),
                                               indices.size() * sizeof(Index));

  // Patch up material settings based on loaded textures
  for (auto index = 0U; index < materials.size(); index++) {
    auto& material = materials.at(index);
    material->set("albedo_map", white_texture);
    material->set("normal_map", white_texture);
    material->set("specular_map", white_texture);
    material->set("roughness_map", white_texture);

    if (!output_images.contains(index)) {
      continue;
    }

    auto& current_images = output_images.at(index);

    if (current_images.contains(TextureType::Albedo)) {
      material->override_property("albedo_map",
                                  current_images.at(TextureType::Albedo));
    }
    if (current_images.contains(TextureType::Normal)) {
      material->override_property("normal_map",
                                  current_images.at(TextureType::Normal));
      material->set("mat_pc.use_normal_map", true);
    }
    if (current_images.contains(TextureType::Specular)) {
      material->override_property("specular_map",
                                  current_images.at(TextureType::Specular));
    }
    if (current_images.contains(TextureType::Roughness)) {
      material->override_property("roughness_map",
                                  current_images.at(TextureType::Roughness));
    }
  }

  // Clear out the staging buffers
  output_images.clear();
}

MeshAsset::~MeshAsset() = default;

auto
MeshAsset::traverse_nodes(aiNode* node,
                          const glm::mat4& parent_transform,
                          Core::u32 level) -> void
{
  if (node == nullptr) {
    return;
  }

  auto local_transform = glm::identity<glm::mat4>();
  local_transform[1][1] *= -1;
  glm::mat4 transform = parent_transform * local_transform;
  ai_node_map[node].resize(node->mNumMeshes);
  for (Core::u32 i = 0; i < node->mNumMeshes; i++) {
    Core::u32 mesh = node->mMeshes[i];
    auto& submesh = submeshes[mesh];
    submesh.node_name = node->mName.C_Str();
    submesh.transform = transform;
    submesh.local_transform = local_transform;
    ai_node_map[node][i] = mesh;
  }

  const auto span = std::span{ node->mChildren, node->mNumChildren };
  for (const auto& child : span) {
    if (child == nullptr) {
      continue;
    }

    traverse_nodes(child, transform, level + 1);
  }
}

StaticMesh::StaticMesh(Core::Ref<MeshAsset> asset)
  : mesh_asset(std::move(asset))
{
  set_submeshes({});
}

void
StaticMesh::set_submeshes(const std::vector<Core::u32>& new_submeshes)
{
  if (!new_submeshes.empty()) {
    submeshes = new_submeshes;
  } else {
    const auto& old_submeshes = mesh_asset->get_submeshes();
    submeshes.resize(old_submeshes.size());
    for (Core::u32 i = 0; i < old_submeshes.size(); i++) {
      submeshes[i] = i;
    }
  }
}

auto
StaticMesh::construct(const std::string_view path) -> Core::Ref<StaticMesh>
{
  return Core::make_ref<StaticMesh>(
    Core::make_ref<MeshAsset>(std::string{ path }));
}

} // namespace Engine::Graphics
