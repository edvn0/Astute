#include "pch/ApplicationPCH.hpp"

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
                             0.01F,
                             1000.0F })
  , scene(std::make_shared<Engine::Core::Scene>(config.scene_name))
{
  auto& scene_widget = std::get<0>(widgets);
  scene_widget = Engine::Core::make_scope<Widgets::SceneWidget>();
};

auto
AstuteApplication::update(f64 ts) -> void
{
  camera->on_update(static_cast<f32>(ts));

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

static u32 chosen_image{ 0 };

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
                     .image_array_index = chosen_image,
                   });
  });

  UI::scope("Light Environment", [s = scene, &r = renderer]() {
    auto& light_environment = s->get_light_environment();
    const std::string label =
      light_environment.is_perspective ? "Perspective" : "Ortho";
    const std::string inverse_label =
      light_environment.is_perspective ? "Ortho" : "Perspective";
    UI::coloured_text({ 0.1F, 0.9F, 0.6F, 1.0F }, "Current chosen: {}", label);
    ImGui::Checkbox(inverse_label.c_str(), &light_environment.is_perspective);
    if (!light_environment.is_perspective) {
      static f32 scale{ 15.0F };
      static f32 near{ 80.0F };
      static f32 far{ 128.0F };
      ImGui::InputFloat("Scale", &scale);
      ImGui::InputFloat("Near", &near);
      ImGui::InputFloat("Far", &far);
      auto projection = glm::ortho(-scale, scale, -scale, scale, near, far);
      light_environment.shadow_projection = projection;
    } else {
      static f32 fov{ 75.0F };
      static f32 aspect{ 1.778F };
      static f32 near{ 0.1F };
      static f32 far{ 90.0F };
      ImGui::InputFloat("FOV", &fov);
      ImGui::InputFloat("Aspect", &aspect);
      ImGui::InputFloat("Near", &near);
      ImGui::InputFloat("Far", &far);
      auto projection = glm::perspective(glm::radians(fov), aspect, near, far);
      light_environment.shadow_projection = projection;
    }

    auto config = r->get_shadow_cascade_configuration();
    if (ImGui::DragFloat("Near Plane Offset",
                         &config.cascade_near_plane_offset)) {
    }

    if (ImGui::DragFloat("Far Plane Offset",
                         &config.cascade_far_plane_offset)) {
    }

    auto& light_colour = light_environment.colour_and_intensity;
    if (ImGui::DragFloat3(
          "Light colour", glm::value_ptr(light_colour), 0.05F, 0.0F, 1.0F)) {
    }
    if (ImGui::DragFloat("Strength", &light_colour.w, 0.1F, 0.0F, 100.0F)) {
    }

    auto& specular_light_colour =
      light_environment.specular_colour_and_intensity;
    if (ImGui::DragFloat3("Specular Light colour",
                          glm::value_ptr(specular_light_colour),
                          0.05F,
                          0.0F,
                          1.0F)) {
    }
    if (ImGui::DragFloat(
          "Specular Strength", &specular_light_colour.w, 0.1F, 0.0F, 100.0F)) {
    }
  });

  UI::begin("Render pass settings");
  renderer->expose_settings_to_ui();
  UI::end();

  ImGui::PopStyleVar();

  for_each_in_tuple(widgets, [](auto& widget) { widget->interface(); });
}

auto
AstuteApplication::handle_events(Event& event) -> void
{
  EventDispatcher dispatcher{ event };
  dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
    const auto keycode = ev.get_keycode();
    if (keycode == KeyCode::KEY_ESCAPE) {
      get_window().close();
      return true;
    }

    // screenshot
    if (keycode == KeyCode::KEY_F12 || keycode == KeyCode::KEY_PRINT_SCREEN) {
      renderer->screenshot();
      return true;
    }

    if (keycode == KeyCode::KEY_9) {
      chosen_image = chosen_image + 1;
      chosen_image = chosen_image % 4;
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
