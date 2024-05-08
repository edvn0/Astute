#pragma once

#include "graphics/Forward.hpp"

#include "graphics/Pipeline.hpp"

#include "core/FrameBasedCollection.hpp"
#include "core/Types.hpp"

#include <tuple>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class RenderPass
{
public:
  virtual ~RenderPass() = default;

  virtual auto construct() -> void = 0;
  virtual auto on_resize(const Core::Extent& new_size) -> void = 0;
  auto execute(CommandBuffer& command_buffer, bool is_compute = false) -> void;

  auto destruct() -> void
  {
    destruct_impl();

    pass = {};
  }

  auto get_colour_attachment(Core::u32) const -> const Core::Ref<Image>&;
  auto get_depth_attachment(bool) const -> const Core::Ref<Image>&;
  auto get_framebuffer() -> decltype(auto)
  {
    return std::get<Core::Scope<Framebuffer>>(pass);
  }
  auto get_framebuffer() const -> const auto&
  {
    return std::get<Core::Scope<Framebuffer>>(pass);
  }

  struct BlitProperties
  {
    std::optional<Core::u32> colour_attachment_index{};
    bool depth_attachment{ false };
  };
  auto blit_to(const CommandBuffer&,
               const Framebuffer&,
               BlitProperties = {}) -> void;

protected:
  explicit RenderPass(Renderer& input)
    : renderer(input)
  {
  }
  virtual auto is_valid() const -> bool
  {
    return std::get<Core::Scope<Framebuffer>>(pass) &&
           std::get<Core::Scope<Shader>>(pass) &&
           std::get<Core::Scope<IPipeline>>(pass) &&
           std::get<Core::Scope<Material>>(pass);
  }
  virtual auto destruct_impl() -> void = 0;
  virtual auto execute_impl(CommandBuffer&) -> void {};
  virtual auto execute_compute_impl(CommandBuffer&) -> void {};
  virtual auto bind(CommandBuffer& command_buffer) -> void;
  virtual auto unbind(CommandBuffer& command_buffer) -> void;
  auto generate_and_update_descriptor_write_sets(Material&) -> VkDescriptorSet;
  auto get_data() -> auto& { return pass; }
  auto get_data() const -> const auto& { return pass; }
  auto get_renderer() -> Renderer& { return renderer; }
  auto get_mutex() -> auto& { return render_pass_mutex; }

private:
  Renderer& renderer;
  using RenderTuple = std::tuple<Core::Scope<Framebuffer>,
                                 Core::Scope<Shader>,
                                 Core::Scope<IPipeline>,
                                 Core::Scope<Material>>;
  RenderTuple pass{};

  static inline std::mutex render_pass_mutex;
  friend class Renderer;
};

} // namespace Engine::Graphics
