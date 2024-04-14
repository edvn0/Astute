#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Event.hpp"

namespace Engine::Core {

class Camera
{
public:
  enum class ProjectionType
  {
    Perspective,
    Orthographic
  };

private:
  glm::vec3 position;
  glm::vec3 up;
  glm::vec3 front;
  f32 yaw;
  f32 pitch;
  f32 aspect_ratio;
  f32 field_of_view;
  f32 near_clip;
  f32 far_clip;
  ProjectionType projection_type;
  f32 speed;
  f32 mouse_sensitivity;
  bool first_mouse;
  f32 last_x, last_y;
  f32 zoom;

  glm::mat4 view_matrix;
  glm::mat4 projection_matrix;

public:
  Camera(glm::vec3 init_position,
         glm::vec3 init_up,
         f32 init_yaw,
         f32 init_pitch,
         f32 init_aspect_ratio,
         f32 init_fov = 45.0f,
         f32 init_near_clip = 0.1f,
         f32 init_far_clip = 100.0f,
         ProjectionType type = ProjectionType::Perspective,
         f32 init_speed = 2.5f,
         f32 init_mouse_sensitivity = 0.1f)
    : position(init_position)
    , up(glm::normalize(init_up))
    , front(glm::vec3(0.0f, 0.0f, -1.0f))
    , yaw(init_yaw)
    , pitch(init_pitch)
    , aspect_ratio(init_aspect_ratio)
    , field_of_view(init_fov)
    , near_clip(init_near_clip)
    , far_clip(init_far_clip)
    , projection_type(type)
    , speed(init_speed)
    , mouse_sensitivity(init_mouse_sensitivity)
    , first_mouse(true)
    , last_x(0.0f)
    , last_y(0.0f)
  {
    update_camera_vectors();
    update_projection();
  }

  auto handle_events(Event&) -> void;

  auto update_camera_vectors() -> void
  {
    glm::vec3 new_front;
    new_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    new_front.y = sin(glm::radians(pitch));
    new_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(new_front);
    glm::vec3 right =
      glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
    view_matrix = glm::lookAt(position, position + front, up);
  }

  auto update_projection() -> void
  {
    if (projection_type == ProjectionType::Perspective) {
      projection_matrix =
        glm::perspective(glm::radians(zoom), aspect_ratio, near_clip, far_clip);
    } else {
      f32 half_width = zoom * aspect_ratio;
      f32 half_height = zoom;
      projection_matrix = glm::ortho(-half_width,
                                     half_width,
                                     -half_height,
                                     half_height,
                                     near_clip,
                                     far_clip);
    }
  }

  auto get_view_matrix() const -> glm::mat4 { return view_matrix; }

  auto get_projection_matrix() const -> glm::mat4 { return projection_matrix; }

  auto move_forward(f32 delta_time) -> void
  {
    position += speed * front * delta_time;
  }

  auto move_backward(f32 delta_time) -> void
  {
    position -= speed * front * delta_time;
  }

  auto move_left(f32 delta_time) -> void
  {
    position -= glm::normalize(glm::cross(front, up)) * speed * delta_time;
  }

  auto move_right(f32 delta_time) -> void
  {
    position += glm::normalize(glm::cross(front, up)) * speed * delta_time;
  }

  auto process_mouse_movement(f32 x_offset,
                              f32 y_offset,
                              bool constrain_pitch = true) -> void
  {
    x_offset *= mouse_sensitivity;
    y_offset *= mouse_sensitivity;

    yaw += x_offset;
    pitch -= y_offset;

    if (constrain_pitch) {
      if (pitch > 89.0f)
        pitch = 89.0f;
      if (pitch < -89.0f)
        pitch = -89.0f;
    }

    update_camera_vectors();
  }
};

} // namespace Engine::Core
