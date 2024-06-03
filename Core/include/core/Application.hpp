#pragma once

#include "core/Event.hpp"
#include "core/Types.hpp"

#include "graphics/Forward.hpp"

namespace Engine::Core {

class Application
{
public:
  struct RendererConfiguration
  {
    const u32 shadow_pass_size{ 1024 };
  };

  struct Configuration
  {
    const bool headless{ false };
    const Extent size{ 1920, 1080 };
    const bool fullscreen{ false };
    const std::string scene_name{ "Astute Scene" };

    const RendererConfiguration renderer{};
  };

  struct Statistics
  {
    f64 frame_time{ 0.0 };
    f64 frames_per_seconds{ 0.0 };
  };

  explicit Application(const Configuration&);
  virtual ~Application();

  auto run() -> i32;

  virtual auto construct() -> void = 0;
  virtual auto destruct() -> void = 0;
  virtual auto update(f64) -> void = 0;
  virtual auto interpolate(f64) -> void = 0;
  virtual auto handle_events(Event&) -> void = 0;
  virtual auto interface() -> void = 0;
  virtual auto on_resize(const Extent&) -> void;
  virtual auto render() -> void = 0;

  static auto the() -> Application&;

  [[nodiscard]] auto current_frame_index() const -> u32;
  [[nodiscard]] auto get_image_count() const -> u32;
  [[nodiscard]] auto get_swapchain() const -> const Graphics::Swapchain&;
  auto get_swapchain() -> Graphics::Swapchain&;

  static auto defer_destruction(std::function<void()>&& func) -> void
  {
    deferred_destruction.emplace_back(std::move(func));
  }

  static auto submit_post_frame_function(std::function<void()>&& func) -> void
  {
    std::scoped_lock lock(post_frame_mutex);
    post_frame_funcs.emplace_back(std::move(func));
  }

  [[nodiscard]] auto get_statistics() const -> const Statistics&
  {
    return statistics;
  }
  auto get_statistics() -> Statistics& { return statistics; }
  [[nodiscard]] auto get_configuration() const -> const Configuration&
  {
    return config;
  }
  [[nodiscard]] auto get_window() const -> const Graphics::Window&
  {
    return *window;
  }
  auto get_window() -> Graphics::Window& { return *window; }

  // Make singleton
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  auto operator=(const Application&) -> Application& = delete;
  auto operator=(Application&&) -> Application& = delete;

protected:
  auto get_configuration() -> Configuration& { return config; }

private:
  Configuration config{};
  Statistics statistics{};

  static inline std::vector<std::function<void()>> deferred_destruction{};
  static inline std::vector<std::function<void()>> post_frame_funcs{};
  static inline std::mutex post_frame_mutex{};

  Scope<Graphics::Window> window;
  Scope<Graphics::InterfaceSystem> interface_system;
  auto forward_incoming_events(Event&) -> void;

  static inline Application* instance{ nullptr };
};

} // namespace Engine::Core
