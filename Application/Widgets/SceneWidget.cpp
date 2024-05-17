#include "pch/ApplicationPCH.hpp"

#include "SceneWidget.hpp"

namespace Widgets {

auto
SceneWidget::interface() -> void
{
  using namespace Engine::Core;

  UI::scope("Scene", [&]() {
    UI::coloured_text(Vec4{ 0.0F, 1.0F, 0.0F, 1.0F }, "Scene {}", "Scene");
  });
}

}
