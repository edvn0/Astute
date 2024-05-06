#include "AstuteApplication.hpp"

#include <imgui.h>

#include "core/Scene.hpp"
#include "graphics/Window.hpp"

static constexpr auto for_each_in_tuple = [](auto&& tuple, auto&& func) {
  std::apply([&func](auto&&... args) { (func(args), ...); }, tuple);
};

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
  , camera(new EditorCamera{ 79.0F,
                             static_cast<f32>(config.size.width),
                             static_cast<f32>(config.size.height),
                             0.8F,
                             1000.0F })
  , scene(std::make_shared<Engine::Core::Scene>(config.scene_name))
{
  auto& scene_widget = std::get<0>(widgets);
  scene_widget = Engine::Core::make_scope<Widgets::SceneWidget>();
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

  for_each_in_tuple(widgets, [ts](auto& widget) { widget->update(ts); });
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
}

auto
AstuteApplication::interface() -> void
{
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));

  UI::scope("Final Output", [&](f32 w, f32 h) {
    UI::image<f32>(*renderer->get_final_output(),
                   {
                     .extent = { w, h },
                   });
  });

  UI::scope("Output Depth", [&](f32 w, f32 h) {
    UI::image<f32>(*renderer->get_shadow_output_image(),
                   {
                     .extent = { w, h },
                     .flipped = true,
                   });
  });

  UI::scope("Shadow projection", [s = scene]() {
    auto& light_environment = s->get_light_environment();
#ifndef ORTHO
    static f32 left{ -10 };
    static f32 right{ 10 };
    static f32 bottom{ -10 };
    static f32 top{ 10 };
    static f32 near{ 3 };
    static f32 far{ 90 };
    ImGui::InputFloat("Left", &left);
    ImGui::InputFloat("Right", &right);
    ImGui::InputFloat("Bottom", &bottom);
    ImGui::InputFloat("Top", &top);
    ImGui::InputFloat("Near", &near);
    ImGui::InputFloat("Far", &far);
    auto projection = glm::ortho(left, right, bottom, top, near, far);
#else
    static f32 fov{ 45 };
    static f32 aspect{ 1 };
    static f32 near{ 0.1 };
    static f32 far{ 100 };
    ImGui::InputFloat("FOV", &fov);
    ImGui::InputFloat("Aspect", &aspect);
    ImGui::InputFloat("Near", &near);
    ImGui::InputFloat("Far", &far);
    auto projection = glm::perspective(glm::radians(fov), aspect, near, far);
#endif
    light_environment.shadow_projection = projection;
  });

  ImGui::PopStyleVar();

  for_each_in_tuple(widgets, [](auto& widget) { widget->interface(); });
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

  for_each_in_tuple(widgets,
                    [&event](auto& widget) { widget->handle_events(event); });

  camera->on_event(event);
};

auto
AstuteApplication::on_resize(const Extent& ext) -> void
{
  Application::on_resize(ext);
  renderer->on_resize(ext);

  for_each_in_tuple(widgets, [ext](auto& widget) { widget->on_resize(ext); });

  if (auto* editor = dynamic_cast<EditorCamera*>(camera.get())) {
    editor->set_viewport_size(ext);
  }
}

auto
AstuteApplication::construct() -> void
{
  for_each_in_tuple(widgets, [](auto& widget) { widget->construct(); });
}

auto
AstuteApplication::destruct() -> void
{
  renderer->destruct();
  for_each_in_tuple(widgets, [](auto& widget) { widget->destruct(); });

  scene.reset();
  renderer.reset();
}
