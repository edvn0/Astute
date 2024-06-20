#pragma once

#include <future>
#include <queue>
#include <vector>

namespace ED {

template<class Container>
concept ContainerLike = requires(Container c) {
  {
    c.push_back(std::declval<typename Container::value_type>())
  };
  {
    c.pop_back()
  };
  {
    c.front()
  };
  {
    c.back()
  };
  {
    c.empty()
  };
};

template<ContainerLike BaseContainer>
class ResultContainer
{
  using T = typename BaseContainer::value_type;

public:
  explicit ResultContainer(BaseContainer& results)
    : results(results)
  {
  }

  auto push(std::future<T>&& future) -> void
  {
    futures.push(std::move(future));
  }

  auto update() -> void
  {
    while (!futures.empty()) {
      auto& future = futures.front();
      if (future.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {
        std::lock_guard lock(single_access);
        results.push_back(future.get());
        futures.pop();
      } else {
        break;
      }
    }
  }

  [[nodiscard]] auto empty() const -> bool { return futures.empty(); }

private:
  BaseContainer& results;
  std::queue<std::future<T>> futures;
  std::mutex single_access;
  std::jthread update_thread{ [this](const std::stop_token& token) {
    while (!token.stop_requested()) {
      update();
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  } };
};

}
