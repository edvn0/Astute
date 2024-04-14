#include "AstuteApplication.hpp"

#include <imgui.h>

#include "graphics/Window.hpp"

AstuteApplication::~AstuteApplication() = default;

AstuteApplication::AstuteApplication(Application::Configuration config)
  : Application(config)
  , camera(glm::vec3(-5, -5, -5),
           glm::vec3(0, -1, 0),
           0.0F,
           0.0F,
           config.size.aspect_ratio()){};

auto
AstuteApplication::update(f64 ts) -> void
{
  camera.update_camera_vectors();

  switch (scene_state) {
    using enum AstuteApplication::SceneState;
    case Edit:
      // scene->update(ts);
      // scene->render(renderer, ts);
      // Scene will call renderer.begin_scene() and renderer.end_scene()
    case Play:
      // TODO: Implement play mode
      break;
    case Pause:
      // TODO: Implement pause mode
      break;
    case Simulate:
      // TODO: Implement simulate mode
      break;
  }
}

auto
AstuteApplication::interpolate(f64) -> void
{
}

static void
ShowDockingDisabledMessage()
{
  auto& io = ImGui::GetIO();
  ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
  ImGui::Text(
    "Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
  ImGui::SameLine(0.0f, 0.0f);
  if (ImGui::SmallButton("click here"))
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

auto
AstuteApplication::interface() -> void
{
  using namespace Engine::UI;
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  scope("Viewport", [&](auto w, auto h) {
    coloured_text({ 1.0F, 1.0F, 0.1F, 1.0F }, "{}", "Test");
    const auto& statistics = get_statistics();
    coloured_text({ 1.0F, 0.1F, 1.0F, 1.0F },
                  "FPS: {}ms, Frametime: {:5f}Hz",
                  statistics.frames_per_seconds,
                  statistics.frame_time);
    // image(renderer.get_output_image(), w, h);
  });
}

auto
AstuteApplication::handle_events(Event& event) -> void
{
  EventDispatcher dispatcher{ event };
  dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
    if (ev.get_keycode() == KeyCode::KEY_ESCAPE) {
      get_window().close();
      return true;
    }
    return false;
  });

  camera.handle_events(event);
};

auto
AstuteApplication::on_resize(const Extent& ext) -> void
{
  Application::on_resize(ext);
}
