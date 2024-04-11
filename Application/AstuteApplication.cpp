#include "AstuteApplication.hpp"

#include "graphics/Window.hpp"

AstuteApplication::~AstuteApplication() = default;

AstuteApplication::AstuteApplication(Application::Configuration config)
  : Application(config){};

auto AstuteApplication::update(f64) -> void{};
auto AstuteApplication::interpolate(f64) -> void{};

auto
AstuteApplication::interface() -> void{};
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
