#include "pch/CorePCH.hpp"

#include "graphics/Mesh.hpp"

#include "core/Logger.hpp"
#include "graphics/Renderer.hpp"

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
  aiProcess_CalcTangentSpace | // Create binormals/tangents just in case
  aiProcess_Triangulate |      // Make sure we're triangles
  aiProcess_SortByPType |      // Split meshes by primitive type
  aiProcess_GenNormals |       // Make sure we have legit normals
  aiProcess_GenUVCoords |      // Convert UVs if required
                               //		aiProcess_OptimizeGraph |
  aiProcess_OptimizeMeshes |   // Batch draws where possible
  aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs |
  aiProcess_LimitBoneWeights | // If more than N (=4) bone weights, discard
                               // least influencing bones and renormalise sum to
                               // 1
  aiProcess_ValidateDataStructure | // Validation
  aiProcess_GlobalScale; // e.g. convert cm to m for fbx import (and other
                         // formats where cm is native)

namespace Utils {

glm::mat4
Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix)
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
  // imported->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

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
    aabb.min = { FLT_MAX, FLT_MAX, FLT_MAX };
    aabb.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (auto i = 0ULL; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      vertex.position = { mesh->mVertices[i].x,
                          mesh->mVertices[i].y,
                          mesh->mVertices[i].z };
      vertex.normals = { mesh->mNormals[i].x,
                         mesh->mNormals[i].y,
                         mesh->mNormals[i].z };
      aabb.min.x = glm::min(vertex.position.x, aabb.min.x);
      aabb.min.y = glm::min(vertex.position.y, aabb.min.y);
      aabb.min.z = glm::min(vertex.position.z, aabb.min.z);
      aabb.max.x = glm::max(vertex.position.x, aabb.max.x);
      aabb.max.y = glm::max(vertex.position.y, aabb.max.y);
      aabb.max.z = glm::max(vertex.position.z, aabb.max.z);

      if (mesh->HasTangentsAndBitangents()) {
        vertex.tangent = {
          mesh->mTangents[i].x,
          mesh->mTangents[i].y,
          mesh->mTangents[i].z,
        };
        vertex.bitangent = {
          mesh->mBitangents[i].x,
          mesh->mBitangents[i].y,
          mesh->mBitangents[i].z,
        };
      }

      if (mesh->HasTextureCoords(0))
        vertex.uvs = {
          mesh->mTextureCoords[0][i].x,
          mesh->mTextureCoords[0][i].y,
        };

      vertices.push_back(vertex);
    }

    // Indices
    for (auto i = 0; i < mesh->mNumFaces; i++) {
      Index index = {
        mesh->mFaces[i].mIndices[0],
        mesh->mFaces[i].mIndices[1],
        mesh->mFaces[i].mIndices[2],
      };
      indices.push_back(index);

      triangle_cache[m].emplace_back(vertices[index.V1 + submesh.base_vertex],
                                     vertices[index.V2 + submesh.base_vertex],
                                     vertices[index.V3 + submesh.base_vertex]);
    }
  }

  traverse_nodes(scene->mRootNode);

  for (const auto& submesh : submeshes) {
    AABB transformed_submesh_aabb = submesh.bounding_box;
    glm::vec3 min = glm::vec3(submesh.transform *
                              glm::vec4(transformed_submesh_aabb.min, 1.0f));
    glm::vec3 max = glm::vec3(submesh.transform *
                              glm::vec4(transformed_submesh_aabb.max, 1.0f));

    bounding_box.min.x = glm::min(bounding_box.min.x, min.x);
    bounding_box.min.y = glm::min(bounding_box.min.y, min.y);
    bounding_box.min.z = glm::min(bounding_box.min.z, min.z);
    bounding_box.max.x = glm::max(bounding_box.max.x, max.x);
    bounding_box.max.y = glm::max(bounding_box.max.y, max.y);
    bounding_box.max.z = glm::max(bounding_box.max.z, max.z);
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
      shininess = 80.0f;
    }

    if (ai_material->Get(AI_MATKEY_REFLECTIVITY, metalness) !=
        aiReturn_SUCCESS) {
      metalness = 0.0f;
    }

    float roughness = 1.0f - glm::sqrt(shininess / 100.0f);

    bool hasAlbedoMap = ai_material->GetTexture(
                          aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
    bool fallback = !hasAlbedoMap;
    if (hasAlbedoMap) {
      Core::Ref<Image> texture;
      if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str())) {
        Core::DataBuffer buffer{ aiTexEmbedded->mWidth *
                                 aiTexEmbedded->mHeight * 4 };
        buffer.write(aiTexEmbedded->pcData,
                     aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * 4);
        texture = Image::load_from_memory(
          aiTexEmbedded->mWidth, aiTexEmbedded->mHeight, std::move(buffer));
      } else {
        std::filesystem::path path = file_name;
        auto parentPath = path.parent_path();
        parentPath /= std::string(aiTexPath.data);
        std::string texturePath = parentPath.string();
        texture = Image::load_from_file({ .path = texturePath });
      }

      if (texture) {
        materials.at(i)->set("albedo_map", texture);
        materials.at(i)->set("mat_pc.albedo_colour", glm::vec3(1.0f));
      } else {
        fallback = true;
      }
    }

    if (fallback) {
      materials.at(i)->set("albedo_map", white_texture);
    }

    // Normal maps
    bool hasNormalMap = ai_material->GetTexture(
                          aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
    fallback = !hasNormalMap;
    if (hasNormalMap) {
      Core::Ref<Image> texture;
      if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str())) {
        Core::DataBuffer buffer{ aiTexEmbedded->mWidth *
                                 aiTexEmbedded->mHeight * 4 };
        buffer.write(aiTexEmbedded->pcData,
                     aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * 4);
        texture = Image::load_from_memory(
          aiTexEmbedded->mWidth, aiTexEmbedded->mHeight, std::move(buffer));
      } else {
        std::filesystem::path path = file_name;
        auto parentPath = path.parent_path();
        parentPath /= std::string(aiTexPath.data);
        std::string texturePath = parentPath.string();
        texture = Image::load_from_file({ .path = texturePath });
      }

      if (texture) {
        materials.at(i)->set("normal_map", texture);
        materials.at(i)->set("mat_pc.use_normal_map", true);
      } else {
        fallback = true;
      }
    }
    if (fallback) {
      materials.at(i)->set("normal_map", white_texture);
      materials.at(i)->set("mat_pc.use_normal_map", false);
    }

    // Normal maps
    bool hasSpecularMap =
      ai_material->GetTexture(aiTextureType_SPECULAR, 0, &aiTexPath) ==
      AI_SUCCESS;
    fallback = !hasSpecularMap;
    if (hasSpecularMap) {
      Core::Ref<Image> texture;
      if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str())) {
        Core::DataBuffer buffer{ aiTexEmbedded->mWidth *
                                 aiTexEmbedded->mHeight * 4 };
        buffer.write(aiTexEmbedded->pcData,
                     aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * 4);
        texture = Image::load_from_memory(
          aiTexEmbedded->mWidth, aiTexEmbedded->mHeight, std::move(buffer));
      } else {
        std::filesystem::path path = file_name;
        auto parentPath = path.parent_path();
        parentPath /= std::string(aiTexPath.data);
        std::string texturePath = parentPath.string();
        texture = Image::load_from_file({ .path = texturePath });
      }

      if (texture) {
        materials.at(i)->set("specular_map", texture);
      } else {
        fallback = true;
      }
    }
    if (fallback) {
      materials.at(i)->set("specular_map", white_texture);
    }

    // Roughness map
    bool hasRoughnessMap =
      ai_material->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) ==
      AI_SUCCESS;

    aiString combined_roughness_metallic_file;
    ai_material->GetTexture(
      aiTextureType_UNKNOWN, 0, &combined_roughness_metallic_file);

    bool prefer_combined = false;
    if (combined_roughness_metallic_file.length > 0) {
      prefer_combined = true;
      hasRoughnessMap = true;
    }

    fallback = !hasRoughnessMap;
    if (hasRoughnessMap) {
      Core::Ref<Image> texture;

      if (auto aiTexEmbedded = scene->GetEmbeddedTexture(
            prefer_combined ? combined_roughness_metallic_file.C_Str()
                            : aiTexPath.C_Str())) {
        Core::DataBuffer buffer{ aiTexEmbedded->mWidth *
                                 aiTexEmbedded->mHeight * 4 };
        buffer.write(aiTexEmbedded->pcData,
                     aiTexEmbedded->mWidth * aiTexEmbedded->mHeight * 4);
        texture = Image::load_from_memory(
          aiTexEmbedded->mWidth, aiTexEmbedded->mHeight, std::move(buffer));
      } else {
        std::filesystem::path path = file_name;
        auto parentPath = path.parent_path();
        parentPath /= std::string(aiTexPath.data);
        std::string texturePath = parentPath.string();
        texture = Image::load_from_file({ .path = texturePath });
      }

      if (texture) {
        materials.at(i)->set("roughness_map", texture);
        materials.at(i)->set("mat_pc.roughness", 1.0F);
      } else {
        fallback = true;
      }
    }

    if (fallback) {
      materials.at(i)->set("roughness_map", white_texture);
      materials.at(i)->set("mat_pc.roughness.Roughness", roughness);
    }

    i++;
  }

  vertex_buffer = Core::make_scope<VertexBuffer>(std::span{ vertices });
  index_buffer = Core::make_scope<IndexBuffer>(indices.data(),
                                               indices.size() * sizeof(Index));
}

auto
MeshAsset::traverse_nodes(aiNode* node,
                          const glm::mat4& parent_transform,
                          Core::u32 level) -> void
{
  glm::mat4 localTransform = Utils::Mat4FromAIMatrix4x4(node->mTransformation);
  glm::mat4 transform = parent_transform * localTransform;
  ai_node_map[node].resize(node->mNumMeshes);
  for (Core::u32 i = 0; i < node->mNumMeshes; i++) {
    Core::u32 mesh = node->mMeshes[i];
    auto& submesh = submeshes[mesh];
    submesh.node_name = node->mName.C_Str();
    submesh.transform = transform;
    submesh.local_transform = localTransform;
    ai_node_map[node][i] = mesh;
  }

  for (Core::u32 i = 0; i < node->mNumChildren; i++)
    traverse_nodes(node->mChildren[i], transform, level + 1);
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

} // namespace Engine::Graphics
