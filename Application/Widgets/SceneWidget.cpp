#include "pch/ApplicationPCH.hpp"

#include "SceneWidget.hpp"

namespace Widgets {

auto
SceneWidget::interface() -> void
{
  using namespace Engine::Core;

  UI::scope("Scene", [&]() {
    UI::coloured_text(Vec4{ 0.0F, 1.0F, 0.0F, 1.0F }, "Scene {}", "Scene");

    auto transform_view =
      current_scene->view<TransformComponent, const IdentityComponent>();
    for (auto&& [entity, component, identity] : transform_view.each()) {
      UI::scope("Entity", [&]() {
        UI::coloured_text(Vec4{ 1.0F, 0.0F, 0.0F, 1.0F }, "{}", identity.name);
        UI::coloured_text(Vec4{ 1.0F, 1.0F, 0.0F, 1.0F }, "Position");
        ImGui::DragFloat3("##Position", &component.translation.x, 0.1F);
        UI::coloured_text(Vec4{ 1.0F, 1.0F, 0.0F, 1.0F }, "Scale");
        ImGui::DragFloat3("##Scale", &component.scale.x, 0.1F);

        // We store a quaternion in the component, so we need to convert it to
        // euler angles for editing
        auto euler = glm::degrees(glm::eulerAngles(component.rotation));
        UI::coloured_text(Vec4{ euler.x, euler.y, euler.z, 1.0F }, "Rotation");
        if (ImGui::DragFloat3("##Rotation", &euler.x, 0.1F)) {
          // Convert back if valid. Validate well!
          component.rotation = glm::quat(glm::radians(euler));
        }
      });
    }
  });
}

}
