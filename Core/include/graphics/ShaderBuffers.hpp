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
  glm::vec4 colour_and_intensity{};
  glm::vec4 specular_colour_and_intensity{};
  glm::vec3 camera_pos{};

  static constexpr std::string_view name = "RendererUBO";
};

struct ShadowUBO
{
  glm::mat4 view{};
  glm::mat4 proj{};
  glm::mat4 view_proj{};
  glm::vec3 light_pos{};

  static constexpr std::string_view name = "ShadowUBO";
};

struct PointLight
{
  glm::vec3 pos{ 0 };
  float intensity{ 0 };
  glm::vec3 radiance{ 0 };
  float min_radius{ 0 };
  float radius{ 0 };
  float falloff{ 0 };
  float light_size{ 0 };
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
  glm::vec3 pos{ 0 };
  float intensity{ 0 };
  glm::vec3 direction{ 0 };
  float angle_attenuation{ 0 };
  glm::vec3 radiance{ 0 };
  float range{ 0 };
  float angle{ 0 };
  float falloff{ 0 };
  Core::PaddedBool soft_shadows{ false };
  Core::PaddedBool casts_shadows{ true };
};

struct SpotLightUBO
{
  Core::PaddedU32 count{ 0 };
  std::array<SpotLight, max_light_count> lights;

  static constexpr std::string_view name = "SpotLightUBO";
};

} // namespace Engine::Graphics
