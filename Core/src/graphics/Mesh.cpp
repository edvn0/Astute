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

namespace Engine::Graphics {

struct AssimpLogStream : public Assimp::LogStream
{
  static void Initialize()
  {
    if (Assimp::DefaultLogger::isNullLogger()) {
      Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
      Assimp::DefaultLogger::get()->attachStream(
        new AssimpLogStream,
        Assimp::Logger::Debugging | Assimp::Logger::Info |
          Assimp::Logger::Warn | Assimp::Logger::Err);
    }
  }

  virtual void write(const char* message) override
  {
    std::string msg(message);
    if (!msg.empty() && msg[msg.length() - 1] == '\n') {
      msg.erase(msg.length() - 1);
    }
    if (strncmp(message, "Debug", 5) == 0) {
      trace("Assimp: {}", msg);
    } else if (strncmp(message, "Info", 4) == 0) {
      info("Assimp: {}", msg);
    } else if (strncmp(message, "Warn", 4) == 0) {
      warn("Assimp: {}", msg);
    } else {
      error("Assimp: {}", msg);
    }
  }
};

static constexpr Core::u32 mesh_import_flags =
  aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast;

namespace Utils {

glm::mat4
mat4_from_assimp_matrix4(const aiMatrix4x4& matrix)
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
{
  deferred_pbr_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/main_geometry.vert", "Assets/shaders/main_geometry.frag");

  AssimpLogStream::Initialize();

  info("Loading mesh: {0}", file_name.c_str());

  importer = Core::make_scope<Assimp::Importer>();
  importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 100.0F);

  const aiScene* scene = importer->ReadFile(file_name, mesh_import_flags);
  if (!scene) {
    error("Failed to load mesh file: {0}", file_name);
    return;
  }

  ai_scene = scene;

  if (!scene->HasMeshes()) {
    return;
  }

  Core::u32 vertexCount = 0;
  Core::u32 indexCount = 0;

  bounding_box.min = { FLT_MAX, FLT_MAX, FLT_MAX };
  bounding_box.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

  submeshes.reserve(scene->mNumMeshes);
  for (unsigned m = 0; m < scene->mNumMeshes; m++) {
    aiMesh* mesh = scene->mMeshes[m];

    Submesh& submesh = submeshes.emplace_back();
    submesh.base_vertex = vertexCount;
    submesh.base_index = indexCount;
    submesh.material_index = mesh->mMaterialIndex;
    submesh.vertex_count = mesh->mNumVertices;
    submesh.index_count = mesh->mNumFaces * 3;
    submesh.mesh_name = mesh->mName.C_Str();

    vertexCount += mesh->mNumVertices;
    indexCount += submesh.index_count;

    // Vertices
    auto& aabb = submesh.bounding_box;
    static constexpr auto flt_max = std::numeric_limits<float>::max();
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

  std::span scene_mats{ scene->mMaterials, scene->mNumMaterials };
  materials.resize(scene_mats.size());
  const auto& white_texture = Renderer::get_white_texture();
  auto i = 0ULL;
  for (const auto& ai_material : scene_mats) {
    materials.at(i) = Core::make_scope<Material>(Material::Configuration{
      .shader = deferred_pbr_shader.get(),
    });
    aiString aiTexPath;

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

    materials.at(i)->set("albedo_map", white_texture);
    materials.at(i)->set("mat_pc.albedo_colour", glm::vec3(1.0F));
    materials.at(i)->set("mat_pc.emission", 1.0F);

    materials.at(i)->set("normal_map", white_texture);
    materials.at(i)->set("mat_pc.use_normal_map", false);

    materials.at(i)->set("specular_map", white_texture);

    materials.at(i)->set("roughness_map", white_texture);
    materials.at(i)->set("mat_pc.roughness", roughness);

    static auto maybe_load_embedded_texture =
      [k = i](auto& dispatcher,
              TextureType T,
              const aiTexture* embedded_texture,
              auto& stagings,
              auto& outputs,
              auto& mutex) -> void {
      if (embedded_texture == nullptr)
        throw std::runtime_error("No texture");

      const auto key = Key{
        .type = T,
        .index = static_cast<Core::u32>(k),
      };

      Core::DataBuffer buffer{ embedded_texture->mWidth *
                               embedded_texture->mHeight * 4 };
      buffer.write(embedded_texture->pcData,
                   embedded_texture->mWidth * embedded_texture->mHeight * 4);

      auto& staging_buffer = stagings[key];
      staging_buffer = Core::make_scope<StagingBuffer>(std::move(buffer));

      const auto width = embedded_texture->mWidth;
      const auto height = embedded_texture->mHeight;

      dispatcher.dispatch(
        [=, &staging = staging_buffer, &images = outputs, &m = mutex](
          auto* cmd) {
          std::lock_guard lock(m);
          images[key] = Image::load_from_memory(cmd,
                                                width,
                                                height,
                                                *staging,
                                                {
                                                  .use_mips = true,
                                                });
        });
    };

    static auto load_texture_from_file =
      [&, k = i](auto& dispatcher,
                 TextureType T,
                 const std::string& base_path,
                 const std::string& texture_path,
                 auto& stagings,
                 auto& outputs,
                 auto& mutex) -> void {
      const auto key = Key{
        .type = T,
        .index = static_cast<Core::u32>(k),
      };
      std::filesystem::path path = base_path;
      auto parent_path = path.parent_path();
      parent_path /= texture_path;
      std::string real_path = parent_path.string();

      Core::u32 w{};
      Core::u32 h{};

      stagings[key] = Image::load_from_file_into_staging(real_path, &w, &h);
      auto& created = stagings[key];

      dispatcher.dispatch(
        [=, &c = created, &images = outputs, &m = mutex](auto* cmd) {
          std::lock_guard lock(m);
          images[key] = Image::load_from_memory(cmd,
                                                w,
                                                h,
                                                *c,
                                                {
                                                  .use_mips = true,
                                                });
        });
    };

    bool has_albedo_map = ai_material->GetTexture(
                            aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
    if (has_albedo_map) {
      if (const auto* embedded_texture =
            scene->GetEmbeddedTexture(aiTexPath.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    TextureType::Albedo,
                                    embedded_texture,
                                    image_staging_buffers,
                                    output_images,
                                    mutex);
      } else {
        load_texture_from_file(dispatcher,
                               TextureType::Albedo,
                               file_name,
                               aiTexPath.C_Str(),
                               image_staging_buffers,
                               output_images,
                               mutex);
      }
    }

    // Normal maps
    bool has_normal_map = ai_material->GetTexture(
                            aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
    if (has_normal_map) {
      if (auto embedded_texture =
            scene->GetEmbeddedTexture(aiTexPath.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    TextureType::Normal,
                                    embedded_texture,
                                    image_staging_buffers,
                                    output_images,
                                    mutex);
      } else {
        load_texture_from_file(dispatcher,
                               TextureType::Normal,
                               file_name,
                               aiTexPath.C_Str(),
                               image_staging_buffers,
                               output_images,
                               mutex);
      }
    }

    // Normal maps
    bool has_specular_map =
      ai_material->GetTexture(aiTextureType_SPECULAR, 0, &aiTexPath) ==
      AI_SUCCESS;
    if (has_specular_map) {
      if (auto embedded_texture =
            scene->GetEmbeddedTexture(aiTexPath.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    TextureType::Specular,
                                    embedded_texture,
                                    image_staging_buffers,
                                    output_images,
                                    mutex);
      } else {
        load_texture_from_file(dispatcher,
                               TextureType::Specular,
                               file_name,
                               aiTexPath.C_Str(),
                               image_staging_buffers,
                               output_images,
                               mutex);
      }
    }

    // Roughness map
    bool has_roughness_map =
      ai_material->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) ==
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
      if (auto embedded_texture = scene->GetEmbeddedTexture(
            prefer_combined ? combined_roughness_metallic_file.C_Str()
                            : aiTexPath.C_Str())) {
        maybe_load_embedded_texture(dispatcher,
                                    TextureType::Roughness,
                                    embedded_texture,
                                    image_staging_buffers,
                                    output_images,
                                    mutex);
      } else {
        load_texture_from_file(dispatcher,
                               TextureType::Roughness,
                               file_name,
                               prefer_combined
                                 ? combined_roughness_metallic_file.C_Str()
                                 : aiTexPath.C_Str(),
                               image_staging_buffers,
                               output_images,
                               mutex);
      }
    }

    i++;
  }

  dispatcher.execute();

  vertex_buffer = Core::make_scope<VertexBuffer>(std::span{ vertices });
  index_buffer = Core::make_scope<IndexBuffer>(indices.data(),
                                               indices.size() * sizeof(Index));

  // Patch up material settings based on loaded textures
  for (auto index = 0ULL; index < materials.size(); index++) {
    auto& material = materials.at(index);

    const auto albedo_key = Key{
      .type = TextureType::Albedo,
      .index = static_cast<Core::u32>(index),
    };
    const auto normal_key = Key{
      .type = TextureType::Normal,
      .index = static_cast<Core::u32>(index),
    };
    const auto specular_key = Key{
      .type = TextureType::Specular,
      .index = static_cast<Core::u32>(index),
    };
    const auto roughness_key = Key{
      .type = TextureType::Roughness,
      .index = static_cast<Core::u32>(index),
    };

    material->set("albedo_map",
                  output_images[albedo_key] != white_texture
                    ? output_images[albedo_key]
                    : white_texture);
    material->set("normal_map",
                  output_images[normal_key] != white_texture
                    ? output_images[normal_key]
                    : white_texture);
    material->set("specular_map",
                  output_images[specular_key] != white_texture
                    ? output_images[specular_key]
                    : white_texture);
    material->set("roughness_map",
                  output_images[roughness_key] != white_texture
                    ? output_images[roughness_key]
                    : white_texture);

    material->set("mat_pc.use_normal_map",
                  output_images[normal_key] != white_texture);
  }

  // Clear out the staging buffers
  image_staging_buffers.clear();
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

  info("Node: {0} Level: {1}", node->mName.C_Str(), level);

  glm::mat4 local_transform =
    Utils::mat4_from_assimp_matrix4(node->mTransformation);
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
    if (!child) {
      continue;
    }

    traverse_nodes(child, transform, level + 1);
  }
}

StaticMesh::StaticMesh(Core::Ref<MeshAsset> asset)
  : mesh_asset(asset)
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
    for (Core::u32 i = 0; i < old_submeshes.size(); i++)
      submeshes[i] = i;
  }
}

auto
StaticMesh::construct(const std::string_view path) -> Core::Ref<StaticMesh>
{
  return Core::make_ref<StaticMesh>(
    Core::make_ref<MeshAsset>(std::string{ path }));
}

} // namespace Engine::Graphics
