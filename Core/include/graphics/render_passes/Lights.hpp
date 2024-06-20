#pragma once

#include "graphics/GPUBuffer.hpp"
#include "graphics/RenderPass.hpp"

#include <glm/glm.hpp>

namespace Engine::Graphics {

class LightsRenderPass final : public RenderPass
{
public:
  explicit LightsRenderPass(Renderer&);
  ~LightsRenderPass() override = default;
  auto on_resize(const Core::Extent&) -> void override;

protected:
  auto construct_impl() -> void override;
  auto destruct_impl() -> void override {}
  auto execute_impl(CommandBuffer&) -> void override;
  auto name() -> std::string_view override { return "Lights"; }

private:
  StorageBuffer storage_buffer;
};

} // namespace Engine::Graphics
