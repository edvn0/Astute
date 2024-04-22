#pragma once

#include "core/Application.hpp"
#include "core/Types.hpp"

#include <unordered_map>

namespace Engine::Core {

inline auto
current_index() -> Core::u32
{
  return Application::the().current_frame_index();
}

template<class Type>
class FrameBasedCollection
{
public:
  FrameBasedCollection()
  {
    for (auto i = 0U; i < Application::the().get_image_count(); i++) {
      collection[i] = {};
    }
  }

  auto get() -> decltype(auto) { return collection.at(current_index()); }
  auto operator*() -> decltype(auto) { return get(); }
  auto operator[](auto index) -> decltype(auto) { return collection.at(index); }

  template<class... Args>
  auto emplace(Args&&... args) -> decltype(auto)
  {
    return collection.emplace<Type>(std::forward<Args>(args)...);
  }

  auto clear()
  {
    if constexpr (requires(Type& t) { t.clear(); }) {
      for (auto i = 0U; i < Application::the().get_image_count(); i++) {
        collection[i].clear();
      }
    } else {
      collection.clear();
      for (auto i = 0U; i < Application::the().get_image_count(); i++) {
        collection[i] = {};
      }
    }
  }

  auto for_each(auto&& func)
  {
    for (auto& [key, value] : collection) {
      func(key, value);
    }
  }

private:
  std::unordered_map<Core::u32, Type> collection;
};

} // namespace Engine::Core
