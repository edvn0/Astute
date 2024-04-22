#include "AstuteApplication.hpp"

#include <imgui.h>

#include "graphics/Window.hpp"

auto
map_to_renderer_config(Application::Configuration config)
  -> Renderer::Configuration
{
  Renderer::Configuration renderer_config;
  renderer_config.shadow_pass_size = config.renderer.shadow_pass_size;
  return renderer_config;
}

AstuteApplication::~AstuteApplication() = default;

AstuteApplication::AstuteApplication(Application::Configuration config)
  : Application(config)
  , renderer(new Renderer{ map_to_renderer_config(config), &get_window() })
  , camera(new EditorCamera{ 45.0F,
                             static_cast<f32>(config.size.width),
                             static_cast<f32>(config.size.height),
                             0.1F,
                             1000.0F })
  , scene(std::make_shared<Scene>(config.scene_name)){

  };

auto
AstuteApplication::update(f64 ts) -> void
{
  camera->on_update(ts);

  switch (scene_state) {
    using enum AstuteApplication::SceneState;
    case Edit:
      scene->on_update_editor(ts);
      break;
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

auto
AstuteApplication::render() -> void
{
  switch (scene_state) {
    using enum AstuteApplication::SceneState;
    case Edit:
      scene->on_render_editor(*renderer, *camera);
      break;
    default:
      break;
  }

  vkDeviceWaitIdle(Device::the().device());
}

auto
AstuteApplication::interface() -> void
{
  using namespace Engine::UI;
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
  scope("Output Position", [&](f32 w, f32 h) {
    image<f32>(*renderer->get_output_image(), { .extent = { w, h } });
  });

  scope("Output Normals", [&](f32 w, f32 h) {
    image<f32>(*renderer->get_output_image(1), { .extent = { w, h } });
  });

  scope("Output Albedo + Spec", [&](f32 w, f32 h) {
    image<f32>(*renderer->get_output_image(2), { .extent = { w, h } });
  });

  scope("Output Depth", [&](f32 w, f32 h) {
    image<f32>(*renderer->get_shadow_output_image(), { .extent = { w, h } });
  });

  scope("Final Output", [&](f32 w, f32 h) {
    image<f32>(*renderer->get_final_output(), { .extent = { w, h } });
  });

  ImGui::PopStyleVar();
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

  camera->on_event(event);
};

auto
AstuteApplication::on_resize(const Extent& ext) -> void
{
  Application::on_resize(ext);
  renderer->on_resize(ext);

  if (auto* editor = dynamic_cast<EditorCamera*>(camera.get())) {
    editor->set_viewport_size(ext);
  }
}

auto
AstuteApplication::destruct() -> void
{
  renderer->destruct();

  scene.reset();
  renderer.reset();
}
