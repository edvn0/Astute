#pragma once

#include "core/Types.hpp"

#include <glm/glm.hpp>

#include <string_view>

namespace Engine::Graphics {

static constexpr auto max_light_count = 1000;

struct RendererUBO
{
  glm::mat4 view{};
  glm::mat4 proj{};
  glm::mat4 view_proj{};
  glm::vec4 colour_and_intensity{ 0.5F, 0.5F, 0.5F, 2.0F };
  glm::vec4 specular_colour_and_intensity{ 0.5F, 0.5F, 0.5F, 2.0F };
  glm::vec4 cascade_splits{};
  glm::vec3 camera_pos{};

  static constexpr std::string_view name = "RendererUBO";
};

struct ShadowUBO
{
  glm::mat4 view{};
  glm::mat4 proj{};
  glm::mat4 view_proj{};
  glm::vec4 light_pos{};
  glm::vec4 light_dir{};

  static constexpr std::string_view name = "ShadowUBO";
};

struct PointLight
{
  glm::vec3 pos{ 0, 5, 0 }; // Slightly elevated
  Core::f32 intensity{ 1.0F };
  glm::vec3 radiance{ 1.0, 1.0, 1.0 }; // White light
  Core::f32 min_radius{ 0.0F };
  Core::f32 radius{ 10.0F };
  Core::f32 falloff{ 2.0F };
  Core::f32 light_size{ 0.1F };
  Core::PaddedBool casts_shadows{ true };
};

struct PointLightUBO
{
  Core::PaddedU32 count{ 0 };
  std::array<PointLight, max_light_count> lights;

  static constexpr std::string_view name = "PointLightUBO";
};

struct SpotLight
{
  glm::vec3 pos{ 0, 10, 0 };
  Core::f32 intensity{ 1.0F };
  glm::vec3 direction{ 0, -1, 0 };
  Core::f32 angle_attenuation{ 3.0F };
  glm::vec3 radiance{ 1.0, 1.0, 1.0 };
  Core::f32 range{ 10.0F };
  Core::f32 angle{ 45.0F };
  Core::f32 falloff{ 2.0F };
  Core::PaddedBool soft_shadows{ false };
  Core::PaddedBool casts_shadows{ true };
};

struct SpotLightUBO
{
  Core::PaddedU32 count{ 0 };
  std::array<SpotLight, max_light_count> lights;

  static constexpr std::string_view name = "SpotLightUBO";
};

struct VisiblePointLightSSBO
{
  std::array<Core::i32, max_light_count> indices;
  static constexpr std::string_view name = "VisiblePointLightSSBO";
};

struct VisibleSpotLightSSBO
{
  std::array<Core::i32, max_light_count> indices;
  static constexpr std::string_view name = "VisibleSpotLightSSBO";
};

struct ScreenDataUBO
{
  glm::vec2 full_resolution{};
  glm::vec2 half_resolution{};
  glm::vec2 inv_resolution{};
  glm::vec2 depth_constants{};
  Core::f32 near_plane{};
  Core::f32 far_plane{};
  Core::f32 time{};
  Core::u32 tile_count_x{};
  static constexpr std::string_view name = "ScreenDataUBO";
};

struct DirectionalShadowProjectionUBO
{
  std::array<glm::mat4, 4> view_projections;
  static constexpr std::string_view name = "DirectionalShadowProjectionUBO";
};

} // namespace Engine::Graphics
