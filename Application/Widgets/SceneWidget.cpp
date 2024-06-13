#include "pch/ApplicationPCH.hpp"

#include "SceneWidget.hpp"

namespace Widgets {

auto
SceneWidget::interface() -> void
{
  using namespace Engine::Core;

  UI::scope("Scene", [&]() {
    UI::coloured_text({ 0.0F, 1.0F, 0.0F, 1.0F }, "Scene {}", "Scene");
    if (current_entity) {
      UI::coloured_text(
        { 0.8F, 0.1F, 0.9F, 1.0F }, "{}", static_cast<u32>(*current_entity));
    } else {
      UI::text("No selected entity");
    }
  });
}

}
