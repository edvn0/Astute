#include "AstuteApplication.hpp"

#include <imgui.h>

AstuteApplication::~AstuteApplication() = default;

AstuteApplication::AstuteApplication(Application::Configuration config)
  : Application(config){};

auto AstuteApplication::update(f64) -> void{};
auto AstuteApplication::interpolate(f64) -> void{};

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
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  Engine::UI::scope("Viewport", [&](auto w, auto h) {
    Engine::UI::coloured_text({ 1.0F, 1.0F, 0.1F, 1.0F }, "{}", "Test");
    const auto& statistics = get_statistics();
    Engine::UI::coloured_text({ 1.0F, 0.1F, 1.0F, 1.0F },
                              "FPS: {}ms, Frametime: {:5f}Hz",
                              statistics.frames_per_seconds,
                              statistics.frame_time);
  });
}

auto
AstuteApplication::handle_events(Event& event) -> void
{
  EventDispatcher dispatcher{ event };
  dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) -> bool {
    if (event.get_keycode() == KeyCode::KEY_ESCAPE) {
      get_window().close();
      return true;
    }
    return false;
  });
};

auto
AstuteApplication::on_resize(const Extent& ext) -> void
{
  Application::on_resize(ext);
}
