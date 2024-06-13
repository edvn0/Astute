#pragma once

#include "graphics/Forward.hpp"

#include "graphics/IFramebuffer.hpp"

#include "graphics/Pipeline.hpp"

#include "core/FrameBasedCollection.hpp"
#include "core/Profiler.hpp"
#include "core/Types.hpp"

#include <tuple>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class RenderPassSettings
{
public:
  virtual ~RenderPassSettings() = default;
  virtual auto expose_to_ui(Material&) -> void = 0;
  virtual auto apply_to_material(Material&) -> void {}
};

class RenderPass
{
public:
  virtual ~RenderPass() = default;

  virtual auto on_resize(const Core::Extent& new_size) -> void = 0;
  auto execute(CommandBuffer& command_buffer) -> void;

  auto destruct() -> void
  {
    destruct_impl();

    pass = {};
  }
  auto construct() -> void;

  [[nodiscard]] auto get_colour_attachment(Core::u32) const
    -> const Core::Ref<Image>&;
  [[nodiscard]] auto get_depth_attachment() const -> const Core::Ref<Image>&;
  auto get_framebuffer() -> decltype(auto)
  {
    return std::get<Core::Scope<IFramebuffer>>(pass);
  }

  /// @brief Some renderpasses can have several framebuffers apart from the
  /// primary one. Indexing starts at zero - the default pass is receivable from
  /// get_framebuffer(/*no-params*/).
  /// @param index The index of the framebuffer to get.
  /// @return The framebuffer.
  virtual auto get_extraneous_framebuffer(Core::u32)
    -> Core::Scope<IFramebuffer>&
  {
    return std::get<Core::Scope<IFramebuffer>>(pass);
  }
  [[nodiscard]] virtual auto get_extraneous_framebuffer(Core::u32) const
    -> const Core::Scope<IFramebuffer>&
  {
    return std::get<Core::Scope<IFramebuffer>>(pass);
  }
  [[nodiscard]] auto get_framebuffer() const -> const auto&
  {
    return std::get<Core::Scope<IFramebuffer>>(pass);
  }

  auto expose_settings_to_ui() -> void
  {
    if (settings) {
      const auto& material = std::get<Core::Scope<Material>>(pass);
      settings->expose_to_ui(*material);
    }
  }

  struct BlitProperties
  {
    std::optional<Core::u32> colour_attachment_index{};
    bool depth_attachment{ false };
  };
  auto blit_to(const CommandBuffer&, const Framebuffer&, BlitProperties)
    -> void;

protected:
  explicit RenderPass(Renderer&);
  [[nodiscard]] virtual auto is_valid() const -> bool
  {
    return std::get<Core::Scope<IFramebuffer>>(pass) &&
           std::get<Core::Scope<Shader>>(pass) &&
           std::get<Core::Scope<IPipeline>>(pass) &&
           std::get<Core::Scope<Material>>(pass);
  }
  virtual auto construct_impl() -> void = 0;
  virtual auto destruct_impl() -> void{};
  virtual auto execute_impl(CommandBuffer&) -> void{};
  virtual auto bind(CommandBuffer& command_buffer) -> void;
  virtual auto unbind(CommandBuffer& command_buffer) -> void;
  auto generate_and_update_descriptor_write_sets(Material&) -> VkDescriptorSet;
  auto get_data() -> auto& { return pass; }
  auto get_material() -> auto& { return std::get<Core::Scope<Material>>(pass); }
  [[nodiscard]] auto get_data() const -> const auto& { return pass; }
  auto get_renderer() -> Renderer& { return renderer; }
  static auto get_mutex() -> auto& { return render_pass_mutex; }
  template<class T>
    requires(std::is_base_of_v<RenderPassSettings, T> &&
             std::is_default_constructible_v<T>)
  auto create_settings() -> void
  {
    settings = Core::make_scope<T>();
  }
  [[nodiscard]] auto get_settings() const -> const auto*
  {
    return settings.get();
  }
  auto get_settings() -> auto* { return settings.get(); }
  template<class T>
    requires std::is_base_of_v<RenderPassSettings, T>
  auto get_current_settings() -> auto*
  {
    return static_cast<T*>(get_settings());
  }
  template<class T>
    requires std::is_base_of_v<RenderPassSettings, T>
  auto get_current_settings() const -> const auto*
  {
    return static_cast<T*>(get_settings());
  }

private:
  Renderer& renderer;
  using RenderTuple = std::tuple<Core::Scope<IFramebuffer>,
                                 Core::Scope<Shader>,
                                 Core::Scope<IPipeline>,
                                 Core::Scope<Material>>;
  RenderTuple pass{};
  bool is_compute{ false };
  Core::Scope<RenderPassSettings> settings{ nullptr };

  static inline std::mutex render_pass_mutex;
  friend class Renderer;
};

} // namespace Engine::Graphics
