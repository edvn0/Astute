#pragma once
#include "graphics/RenderPass.hpp"

namespace filewatch {
enum class Event : std::int32_t;
}

namespace Engine::Graphics {

struct Impl;

class DeferredRenderPass final : public RenderPass
{
public:
  explicit DeferredRenderPass(Renderer& renderer,
                              const Core::Ref<Image>& cubemap);
  ~DeferredRenderPass() override;
  auto on_resize(const Core::Extent& extent) -> void override;
  auto set_cubemap(const Core::Ref<Image>& new_cubemap)
  {
    cubemap = new_cubemap;
  }

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer& command_buffer) -> void override;
  auto name() -> std::string_view override { return "Deferred"; }

private:
  Core::Scope<Impl> watch;
  auto setup_file_watcher(const std::string& shader_path,
                          bool is_cubemap = false) -> void;
  auto handle_file_change(const std::string& path,
                          filewatch::Event change_type,
                          bool is_cubemap) -> void;
  auto log_shader_change(const std::string& path, filewatch::Event change_type)
    -> void;
  auto reload_shader(bool is_cubemap) -> void;
  auto recreate_pipeline(bool is_cubemap = false) -> void;

  Core::Ref<Image> noise_map;
  Core::Ref<Image> cubemap;

  // New members for cubemap pass
  Core::Scope<Shader> cubemap_shader;
  Core::Scope<GraphicsPipeline> cubemap_pipeline;
  Core::Scope<Material> cubemap_material;

  auto create_noise_map() -> void;
  auto create_framebuffer() -> void;
  auto create_deferred_pass() -> void;
  auto create_cubemap_pass() -> void;
  auto set_material_uniforms() -> void;
  auto setup_file_watchers() -> void;
};

} // namespace Engine::Graphics
