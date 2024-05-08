#pragma once

#include "graphics/RenderPass.hpp"

namespace filewatch {
template<typename T>
class FileWatch;

enum class Event : std::int32_t;
} // namespace filewatch

namespace Engine::Graphics {

class DeferredRenderPass final : public RenderPass
{
public:
  explicit DeferredRenderPass(Renderer& ren)
    : RenderPass(ren)
  {
    setup_file_watcher("Assets/shaders/deferred.frag");
  }
  ~DeferredRenderPass() override { watch.reset(); }
  auto construct() -> void override;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto destruct_impl() -> void override;
  auto execute_impl(CommandBuffer&) -> void override;

private:
  Core::Scope<filewatch::FileWatch<std::string>> watch;

  auto setup_file_watcher(const std::string& shader_path) -> void;
  auto handle_file_change(const std::string& path,
                          filewatch::Event change_type) -> void;
  auto log_shader_change(const std::string& path, filewatch::Event change_type) -> void;
  auto reload_shader() -> void;
  auto recreate_pipeline() -> void;
};

} // namespace Engine::Graphics
