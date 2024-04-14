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
  float yaw;
  float pitch;
  float aspect_ratio;
  float field_of_view;
  float near_clip;
  float far_clip;
  ProjectionType projection_type;
  float speed;
  float mouse_sensitivity;
  bool first_mouse;
  float last_x, last_y;
  float zoom;

  glm::mat4 view_matrix;
  glm::mat4 projection_matrix;

public:
  Camera(glm::vec3 init_position,
         glm::vec3 init_up,
         float init_yaw,
         float init_pitch,
         float init_aspect_ratio,
         float init_fov = 45.0f,
         float init_near_clip = 0.1f,
         float init_far_clip = 100.0f,
         ProjectionType type = ProjectionType::Perspective,
         float init_speed = 2.5f,
         float init_mouse_sensitivity = 0.1f)
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
    // Calculate the new Front vector
    glm::vec3 new_front;
    new_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    new_front.y = sin(glm::radians(pitch));
    new_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(new_front);
    // Recalculate the Right and Up vector
    glm::vec3 right = glm::normalize(
      glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))); // Fixed up vector
    up = glm::normalize(glm::cross(right, front));
    // Update view matrix
    view_matrix = glm::lookAt(position, position + front, up);
  }

  auto update_projection() -> void
  {
    if (projection_type == ProjectionType::Perspective) {
      projection_matrix =
        glm::perspective(glm::radians(zoom), aspect_ratio, near_clip, far_clip);
    } else {
      float half_width = zoom * aspect_ratio;
      float half_height = zoom;
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

  auto move_forward(float delta_time) -> void
  {
    position += speed * front * delta_time;
  }

  auto move_backward(float delta_time) -> void
  {
    position -= speed * front * delta_time;
  }

  auto move_left(float delta_time) -> void
  {
    position -= glm::normalize(glm::cross(front, up)) * speed * delta_time;
  }

  auto move_right(float delta_time) -> void
  {
    position += glm::normalize(glm::cross(front, up)) * speed * delta_time;
  }

  auto process_mouse_movement(float x_offset,
                              float y_offset,
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
