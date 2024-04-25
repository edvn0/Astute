#pragma once

#include "graphics/Forward.hpp"

#include "core/Types.hpp"

#include <tuple>

namespace Engine::Graphics {

class RenderPass
{
public:
  virtual ~RenderPass() = default;

  virtual auto update_attachment(Core::u32, const Core::Ref<Image>&) -> void;
  virtual auto construct(Renderer&) -> void;

  virtual auto on_resize(const Core::Extent& new_size) -> void = 0;
  auto destruct() -> void
  {
    destruct_impl();

    if (framebuffer)
      framebuffer.reset();
    if (shader)
      shader.reset();
    if (pipeline)
      pipeline.reset();
    if (material)
      material.reset();
  }

protected:
  virtual auto destruct_impl() -> void;
  auto get_data() -> std::tuple<Core::Scope<Framebuffer>&,
                                Core::Scope<Shader>&,
                                Core::Scope<GraphicsPipeline>&,
                                Core::Scope<Material>&>
  {
    return std::tuple<Core::Scope<Framebuffer>&,
                      Core::Scope<Shader>&,
                      Core::Scope<GraphicsPipeline>&,
                      Core::Scope<Material>&>{
      framebuffer, shader, pipeline, material
    };
  }

private:
  Core::Scope<Framebuffer> framebuffer{ nullptr };
  Core::Scope<Shader> shader{ nullptr };
  Core::Scope<GraphicsPipeline> pipeline{ nullptr };
  Core::Scope<Material> material{ nullptr };

  friend class Renderer;
};

} // namespace Engine::Graphics
