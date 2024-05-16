#pragma once

#include "core/Types.hpp"

namespace Engine::Graphics {

enum class QueueType : Core::u8
{
  Graphics,
  Compute,
  Present,
  Transfer
};

} // namespace Engine::Graphics
