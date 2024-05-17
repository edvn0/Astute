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
  explicit DeferredRenderPass(Renderer&);
  ~DeferredRenderPass() override;
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;

private:
  Core::Scope<Impl> watch;

  auto setup_file_watcher(const std::string& shader_path) -> void;
  auto handle_file_change(const std::string& path, filewatch::Event change_type)
    -> void;
  auto log_shader_change(const std::string& path, filewatch::Event change_type)
    -> void;
  auto reload_shader() -> void;
  auto recreate_pipeline() -> void;

  Core::Ref<Image> noise_map;
};

} // namespace Engine::Graphics
