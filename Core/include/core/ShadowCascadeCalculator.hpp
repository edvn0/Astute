#pragma once

#include "core/Camera.hpp"
#include "core/Types.hpp"
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Engine::Core {

struct CascadeData
{
  glm::mat4 view_projection;
  glm::mat4 view;
  Core::f32 split_depth;
};

class ShadowCascadeCalculator
{
  static constexpr auto shadow_map_cascade_count = 4ULL;
  static constexpr float cascade_split_lambda = 0.95f;
  static constexpr float shadow_resolution = 4096.0f;
  static constexpr std::array<glm::vec3, 8> frustum_corners = {
    glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, 1.0f, 1.0f),  glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f),  glm::vec3(-1.0f, -1.0f, 1.0f),
  };

  Core::f32& cascade_near_plane_offset;
  Core::f32& cascade_far_plane_offset;

public:
  ShadowCascadeCalculator(Core::f32& cascade_near_plane_offset,
                          Core::f32& cascade_far_plane_offset)
    : cascade_near_plane_offset(cascade_near_plane_offset)
    , cascade_far_plane_offset(cascade_far_plane_offset)
  {
  }

  auto get_editable_near_plane_offset() -> Core::f32&
  {
    return cascade_near_plane_offset;
  }

  auto compute_cascades(const SceneRendererCamera& camera,
                        const glm::vec3& light_direction)
    -> std::array<CascadeData, shadow_map_cascade_count>
  {
    std::array<CascadeData, shadow_map_cascade_count> output{};
    std::array<float, shadow_map_cascade_count> cascade_splits =
      calculate_cascade_splits(camera.near, camera.far);

    float last_split_dist = 0.0f;
    for (Core::u32 i = 0; i < shadow_map_cascade_count; i++) {
      float split_dist = cascade_splits[i];

      std::array<glm::vec3, 8> frustum_corners_world =
        calculate_frustum_corners_world(camera, split_dist, last_split_dist);

      auto [min_extents, max_extents, frustum_center] =
        calculate_frustum_bounds(frustum_corners_world);

      glm::mat4 light_view_matrix = calculate_light_view_matrix(
        frustum_center, light_direction, min_extents);
      glm::mat4 light_orthographic_matrix =
        calculate_light_orthographic_matrix(min_extents, max_extents);
      light_orthographic_matrix =
        adjust_shadow_matrix(light_orthographic_matrix, shadow_resolution);

      output[i].split_depth =
        (camera.near + split_dist * (camera.far - camera.near)) * -1.0f;
      output[i].view = light_view_matrix;
      output[i].view_projection = light_orthographic_matrix * light_view_matrix;

      last_split_dist = cascade_splits[i];
    }

    return output;
  }

private:
  auto calculate_cascade_splits(float near_clip, float far_clip)
    -> std::array<float, shadow_map_cascade_count>
  {
    std::array<float, shadow_map_cascade_count> cascade_splits{};
    float clip_range = far_clip - near_clip;
    float min_z = near_clip;
    float max_z = near_clip + clip_range;
    float range = max_z - min_z;
    float ratio = max_z / min_z;

    for (Core::u32 i = 0; i < shadow_map_cascade_count; i++) {
      float p = (static_cast<float>(i) + 1) /
                static_cast<float>(shadow_map_cascade_count);
      float log = min_z * std::pow(ratio, p);
      float uniform = min_z + range * p;
      float d = cascade_split_lambda * (log - uniform) + uniform;
      cascade_splits[i] = (d - near_clip) / clip_range;
    }

    return cascade_splits;
  }

  auto calculate_frustum_corners_world(const SceneRendererCamera& camera,
                                       float split_dist,
                                       float last_split_dist)
    -> std::array<glm::vec3, 8>
  {
    std::array<glm::vec3, 8> frustum_corners_world = frustum_corners;

    glm::mat4 inv_cam =
      glm::inverse(camera.camera.get_projection_matrix() * camera.view_matrix);
    for (auto& corner : frustum_corners_world) {
      glm::vec4 inv_corner = inv_cam * glm::vec4(corner, 1.0f);
      corner = glm::vec3(inv_corner) / inv_corner.w;
    }

    for (uint32_t j = 0; j < 4; j++) {
      glm::vec3 dist = frustum_corners_world[j + 4] - frustum_corners_world[j];
      frustum_corners_world[j + 4] =
        frustum_corners_world[j] + (dist * split_dist);
      frustum_corners_world[j] =
        frustum_corners_world[j] + (dist * last_split_dist);
    }

    return frustum_corners_world;
  }

  auto calculate_frustum_bounds(
    const std::array<glm::vec3, 8>& frustum_corners_world)
    -> std::tuple<glm::vec3, glm::vec3, glm::vec3>
  {
    glm::vec3 frustum_center = glm::vec3(0.0f);
    for (const auto& corner : frustum_corners_world) {
      frustum_center += corner;
    }
    frustum_center /= 8.0f;

    float radius = 0.0f;
    for (const auto& corner : frustum_corners_world) {
      float distance = glm::length(corner - frustum_center);
      radius = glm::max(radius, distance);
    }
    radius = std::ceil(radius * 16.0f) / 16.0f;

    glm::vec3 max_extents = glm::vec3(radius);
    glm::vec3 min_extents = -max_extents;

    return { min_extents, max_extents, frustum_center };
  }

  auto calculate_light_view_matrix(const glm::vec3& frustum_center,
                                   const glm::vec3& light_direction,
                                   const glm::vec3& min_extents) -> glm::mat4
  {
    glm::vec3 light_dir = -glm::normalize(light_direction);
    return glm::lookAt(frustum_center - light_dir * -min_extents.z,
                       frustum_center,
                       glm::vec3(0.0f, 1.0f, 0.0f));
  }

  auto calculate_light_orthographic_matrix(const glm::vec3& min_extents,
                                           const glm::vec3& max_extents)
    -> glm::mat4
  {
    return glm::ortho(min_extents.x,
                      max_extents.x,
                      min_extents.y,
                      max_extents.y,
                      0.0f + cascade_near_plane_offset,
                      max_extents.z - min_extents.z + cascade_far_plane_offset);
  }

  auto adjust_shadow_matrix(glm::mat4 light_orthographic_matrix,
                            float input_shadow_resolution) -> glm::mat4
  {
    glm::vec4 shadow_origin =
      (light_orthographic_matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) *
      input_shadow_resolution / 2.0f;
    glm::vec4 rounded_origin = glm::round(shadow_origin);
    glm::vec4 round_offset = rounded_origin - shadow_origin;
    round_offset = round_offset * 2.0f / input_shadow_resolution;
    round_offset.z = 0.0f;
    round_offset.w = 0.0f;

    light_orthographic_matrix[3] += round_offset;
    return light_orthographic_matrix;
  }
};

} // namespace Engine::Core
