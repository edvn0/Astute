#include "pch/ApplicationPCH.hpp"

#include "PerformanceWidget.hpp"

namespace Widgets {

auto
PerformanceWidget::interface() -> void
{
  using namespace Engine::Core;

  UI::scope("Scene", [&]() {
    // Extract frame_time and fps data into separate arrays for plotting
    std::array<float, buffer_size> frame_times;
    std::array<float, buffer_size> fps_values;

    for (size_t i = 0; i < buffer_size; ++i) {
      frame_times[i] = static_cast<float>(statistics[i].frame_time);
      fps_values[i] = static_cast<float>(statistics[i].fps);
    }

    const auto as_i32 = static_cast<Engine::Core::i32>(current_index);

    UI::coloured_text({ 1.0, 0.0, 0.0, 1.0 }, "FPS: {:.2F}", mean(fps_values));
    // Display the frame times as a scrolling plot
    ImGui::PlotLines("Frame Times (ms)",
                     frame_times.data(),
                     buffer_size,
                     as_i32,
                     nullptr,
                     0.0F,
                     *std::max_element(frame_times.begin(), frame_times.end()),
                     ImVec2(0, 100));
  });
}

auto
PerformanceWidget::update(Engine::Core::f64) -> void
{
  auto& measurement = statistics[current_index];
  const auto& stats = Engine::Core::Application::the().get_statistics();
  measurement.frame_time = stats.frame_time;
  measurement.fps = stats.frames_per_seconds;
  current_index = (current_index + 1) % buffer_size;
}

}
