#pragma once

#include "core/Types.hpp"
#include "reflection/ReflectionData.hpp"

#include <vector>
#include <vulkan/vulkan.h>

#include "core/Forward.hpp"
#include "graphics/Forward.hpp"

namespace Engine::Reflection {

class Reflector
{
  struct CompilerImpl;

public:
  explicit Reflector(Graphics::Shader&);
  ~Reflector();
  auto reflect(std::vector<VkDescriptorSetLayout>&,
               ReflectionData& output) const -> void;

private:
  Graphics::Shader& shader;
  Core::Scope<CompilerImpl> impl{ nullptr };
};

} // namespace Reflection
