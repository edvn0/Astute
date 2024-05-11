#include "pch/CorePCH.hpp"

#include "core/Camera.hpp"
#include "core/Input.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Engine::Core {

Camera::Camera(const glm::mat4& projection,
               const glm::mat4& unreversed_projection)
  : projection_matrix(projection)
  , unreversed_projection_matrix(unreversed_projection){};

Camera::Camera(const float degree_fov,
               const float width,
               const float height,
               const float near_plane,
               const float far_plane)
  : projection_matrix(glm::perspectiveFov(glm::radians(degree_fov),
                                          width,
                                          height,
                                          far_plane,
                                          near_plane))
  , unreversed_projection_matrix(glm::perspectiveFov(glm::radians(degree_fov),
                                                     width,
                                                     height,
                                                     near_plane,
                                                     far_plane)){};

void
Camera::focus(const glm::vec3&){};

void
Camera::on_update(float){};

void
Camera::set_ortho_projection_matrix(const float width,
                                    const float height,
                                    const float near_plane,
                                    const float far_plane)
{
  static constexpr auto half = 0.5F;
  projection_matrix = glm::ortho(-width * half,
                                 width * half,
                                 -height * half,
                                 height * half,
                                 far_plane,
                                 near_plane);
  unreversed_projection_matrix = glm::ortho(-width * half,
                                            width * half,
                                            -height * half,
                                            height * half,
                                            near_plane,
                                            far_plane);
}

void
Camera::set_perspective_projection_matrix(float radians_fov,
                                          float width,
                                          float height,
                                          float near_plane,
                                          float far_plane)
{
  projection_matrix =
    glm::perspectiveFov(radians_fov, width, height, far_plane, near_plane);
  unreversed_projection_matrix =
    glm::perspectiveFov(radians_fov, width, height, near_plane, far_plane);
}

EditorCamera::EditorCamera(const float degree_fov,
                           const float width,
                           const float height,
                           const float near_plane,
                           const float far_plane,
                           const EditorCamera* previous_camera)
  : Camera(glm::perspectiveFov(glm::radians(degree_fov),
                               width,
                               height,
                               near_plane,
                               far_plane),
           glm::perspectiveFov(glm::radians(degree_fov),
                               width,
                               height,
                               far_plane,
                               near_plane))
  , focal_point(0.0F)
  , vertical_fov(glm::radians(degree_fov))
  , near_clip(near_plane)
  , far_clip(far_plane)
{
  if (previous_camera != nullptr) {
    position = previous_camera->position;
    position_delta = previous_camera->position_delta;
    yaw = previous_camera->yaw;
    yaw_delta = previous_camera->yaw_delta;
    pitch = previous_camera->pitch;
    pitch_delta = previous_camera->pitch_delta;

    focal_point = previous_camera->focal_point;
  }

  distance = glm::distance(position, focal_point);

  position = calculate_position();
  const glm::quat orientation = get_orientation();
  static constexpr auto pi_to_rad = 180.0F / glm::pi<float>();
  direction = glm::eulerAngles(orientation) * pi_to_rad;
  view_matrix =
    glm::translate(glm::mat4(1.0F), position) * glm::mat4(orientation);
}

void
EditorCamera::init(const EditorCamera* previous_camera)
{
  if (previous_camera != nullptr) {
    position = previous_camera->position;
    position_delta = previous_camera->position_delta;
    yaw = previous_camera->yaw;
    yaw_delta = previous_camera->yaw_delta;
    pitch = previous_camera->pitch;
    pitch_delta = previous_camera->pitch_delta;

    focal_point = previous_camera->focal_point;
  }

  distance = glm::distance(position, focal_point);

  position = calculate_position();
  const glm::quat orientation = get_orientation();
  direction = glm::eulerAngles(orientation) * (180.0F / glm::pi<float>());
  view_matrix =
    glm::translate(glm::mat4(1.0F), position) * glm::mat4(orientation);
}

void
EditorCamera::on_update(const float time_step)
{
  auto&& [x, y] = Input::mouse_position();
  const glm::vec2 mouse{ x, y };
  const glm::vec2 delta = (mouse - initial_mouse_position) * 0.002F;

  if (!is_active())
    return;

  if (Input::pressed(MouseCode::MOUSE_BUTTON_RIGHT) &&
      !Input::pressed(KeyCode::KEY_LEFT_CONTROL)) {
    camera_mode = CameraMode::Flycam;
    const float yaw_sign = get_up_direction().y < 0 ? -1.0F : 1.0F;

    const float speed = get_camera_speed();

    if (Input::pressed(KeyCode::KEY_Q)) {
      position_delta -= time_step * speed * glm::vec3{ 0.F, yaw_sign, 0.F };
    }
    if (Input::pressed(KeyCode::KEY_E)) {
      position_delta += time_step * speed * glm::vec3{ 0.F, yaw_sign, 0.F };
    }
    if (Input::pressed(KeyCode::KEY_S)) {
      position_delta -= time_step * speed * direction;
    }
    if (Input::pressed(KeyCode::KEY_W)) {
      position_delta += time_step * speed * direction;
    }
    if (Input::pressed(KeyCode::KEY_A)) {
      position_delta -= time_step * speed * right_direction;
    }
    if (Input::pressed(KeyCode::KEY_D)) {
      position_delta += time_step * speed * right_direction;
    }

    constexpr float max_rate{ 0.12f };
    yaw_delta +=
      glm::clamp(yaw_sign * delta.x * rotation_speed(), -max_rate, max_rate);
    pitch_delta += glm::clamp(delta.y * rotation_speed(), -max_rate, max_rate);

    right_direction = glm::cross(direction, glm::vec3{ 0.F, yaw_sign, 0.F });

    const auto angle_axis_right =
      glm::angleAxis(-yaw_delta, glm::vec3{ 0.F, yaw_sign, 0.F });
    const auto angle_axis_left = glm::angleAxis(-pitch_delta, right_direction);
    const auto cross_product = glm::cross(angle_axis_left, angle_axis_right);
    const auto normalised = glm::normalize(cross_product);
    direction = glm::rotate(normalised, direction);

    const float actual_distance = glm::distance(focal_point, position);
    focal_point = position + get_forward_direction() * actual_distance;
    distance = actual_distance;
  } else if (Input::pressed(KeyCode::KEY_LEFT_CONTROL)) {
    camera_mode = CameraMode::Arcball;

    if (Input::pressed(MouseCode::MOUSE_BUTTON_MIDDLE)) {
      mouse_pan(delta);
    } else if (Input::pressed(MouseCode::MOUSE_BUTTON_LEFT)) {
      mouse_rotate(delta);
    } else if (Input::pressed(MouseCode::MOUSE_BUTTON_RIGHT)) {
      mouse_zoom(delta.x + delta.y);
    }
  }

  initial_mouse_position = mouse;
  position += position_delta;
  yaw += yaw_delta;
  pitch += pitch_delta;

  if (camera_mode == CameraMode::Arcball) {
    position = calculate_position();
  }

  update_camera_view();
}

auto
EditorCamera::get_camera_speed() const -> float
{
  float speed = normal_speed;
  if (Input::pressed(KeyCode::KEY_LEFT_CONTROL)) {
    speed /= 2 - glm::log(normal_speed);
  }
  if (Input::pressed(KeyCode::KEY_LEFT_SHIFT)) {
    speed *= 2 - glm::log(normal_speed);
  }

  return glm::clamp(speed, min_speed, max_speed);
}

void
EditorCamera::update_camera_view()
{
  const float yaw_sign = get_up_direction().y < 0 ? -1.0F : 1.0F;

  const float cos_angle = glm::dot(get_forward_direction(), get_up_direction());
  if (cos_angle * yaw_sign > 0.99F) {
    pitch_delta = 0.F;
  }

  const glm::vec3 look_at = position + get_forward_direction();
  direction = glm::normalize(look_at - position);
  distance = glm::distance(position, focal_point);
  view_matrix = glm::lookAt(position, look_at, glm::vec3{ 0.F, yaw_sign, 0.F });

  yaw_delta *= 0.6F;
  pitch_delta *= 0.6F;
  position_delta *= 0.8F;
}

void
EditorCamera::focus(const glm::vec3& focus_point)
{
  focal_point = focus_point;
  camera_mode = CameraMode::Flycam;
  if (distance > min_focus_distance) {
    distance -= distance - min_focus_distance;
    position = focal_point - get_forward_direction() * distance;
  }
  position = focal_point - get_forward_direction() * distance;
  update_camera_view();
}

auto
EditorCamera::pan_speed() const -> std::pair<float, float>
{
  const float x = glm::min(static_cast<float>(viewport.width) / 1000.0F,
                           2.4F); // max = 2.4f
  const float x_factor = 0.0366f * (x * x) - 0.1778f * x + 0.3021F;

  const float y = glm::min(static_cast<float>(viewport.height) / 1000.0F,
                           2.4F); // max = 2.4f
  const float y_factor = 0.0366f * (y * y) - 0.1778f * y + 0.3021F;

  return { x_factor, y_factor };
}

auto
EditorCamera::rotation_speed() -> float
{
  return 0.3F;
}

auto
EditorCamera::zoom_speed() const -> float
{
  float dist = distance * 0.2F;
  dist = glm::max(dist, 0.0F);
  float speed = dist * dist;
  speed = glm::min(speed, 50.0F); // max speed = 50
  return speed;
}

void
EditorCamera::on_event(Event& event)
{
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseScrolledEvent>(
    [this](MouseScrolledEvent& e) { return on_mouse_scroll(e); });
}

auto
EditorCamera::on_mouse_scroll(MouseScrolledEvent& e) -> bool
{
  if (Input::pressed(MouseCode::MOUSE_BUTTON_RIGHT)) {
    normal_speed += e.get_y_offset() * 0.3f * normal_speed;
    normal_speed = std::clamp(normal_speed, min_speed, max_speed);
  } else {
    mouse_zoom(e.get_y_offset() * 0.1F);
    update_camera_view();
  }

  return true;
}

void
EditorCamera::mouse_pan(const glm::vec2& delta)
{
  auto&& [x_velocity, y_velocity] = pan_speed();
  focal_point -= get_right_direction() * delta.x * x_velocity * distance;
  focal_point += get_up_direction() * delta.y * y_velocity * distance;
}

void
EditorCamera::mouse_rotate(const glm::vec2& delta)
{
  const float yaw_sign = get_up_direction().y < 0.0F ? -1.0F : 1.0F;
  yaw_delta += yaw_sign * delta.x * rotation_speed();
  pitch_delta += delta.y * rotation_speed();
}

void
EditorCamera::mouse_zoom(float delta)
{
  distance -= delta * zoom_speed();
  const glm::vec3 forward_dir = get_forward_direction();
  position = focal_point - forward_dir * distance;
  if (distance < 1.0F) {
    focal_point += forward_dir * distance;
    distance = 1.0F;
  }
  position_delta += delta * zoom_speed() * forward_dir;
}

auto
EditorCamera::get_up_direction() const -> glm::vec3
{
  return glm::rotate(get_orientation(), glm::vec3(0.0F, 1.0F, 0.0F));
}

auto
EditorCamera::get_right_direction() const -> glm::vec3
{
  return glm::rotate(get_orientation(), glm::vec3(1.F, 0.F, 0.F));
}

auto
EditorCamera::get_forward_direction() const -> glm::vec3
{
  return glm::rotate(get_orientation(), glm::vec3(0.0F, 0.0F, -1.0F));
}

auto
EditorCamera::calculate_position() const -> glm::vec3
{
  return focal_point - get_forward_direction() * distance + position_delta;
}

auto
EditorCamera::get_orientation() const -> glm::quat
{
  return glm::vec3(-pitch - pitch_delta, -yaw - yaw_delta, 0.0F);
}
}
