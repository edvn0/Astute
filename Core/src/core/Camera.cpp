#include "pch/CorePCH.hpp"

#include "core/Camera.hpp"

namespace Engine::Core {

auto
Camera::handle_events(Event& event) -> void
{
  EventDispatcher dispatcher(event);

  dispatcher.dispatch<KeyPressedEvent>([this](auto& e) {
    if (e.get_keycode() == KeyCode::KEY_W) {
      move_forward(0.1f); // Assume a fixed delta time for simplicity
    }
    if (e.get_keycode() == KeyCode::KEY_S) {
      move_backward(0.1f);
    }
    if (e.get_keycode() == KeyCode::KEY_A) {
      move_left(0.1f);
    }
    if (e.get_keycode() == KeyCode::KEY_D) {
      move_right(0.1f);
    }
    return false;
  });

  dispatcher.dispatch<MouseMovedEvent>([this](auto& e) {
    static float lastX = 400; // Example initial window center width
    static float lastY = 300; // Example initial window center height
    float xoffset = e.get_x() - lastX;
    float yoffset =
      lastY -
      e.get_y(); // Reversed since y-coordinates range from bottom to top
    lastX = e.get_x();
    lastY = e.get_y();

    process_mouse_movement(xoffset, yoffset);
    return false;
  });

  dispatcher.dispatch<MouseScrolledEvent>([this](auto& e) {
    zoom -= e.get_y_offset();
    if (zoom < 1.0f)
      zoom = 1.0f; // Clamp minimum zoom
    if (zoom > 100.0f)
      zoom = 100.0f;     // Clamp maximum zoom
    update_projection(); // Recompute the projection matrix with new zoom level
    return false;
  });
}
}