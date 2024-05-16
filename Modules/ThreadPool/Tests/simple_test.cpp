#include <gtest/gtest.h>

#include "thread_pool/ThreadPool.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Device.hpp"
#include "graphics/Image.hpp"
#include "graphics/Instance.hpp"

#include "graphics/TextureGenerator.hpp"

#include <array>
#include <ranges>
#include <span>

struct DeviceProvider
{

public:
  ~DeviceProvider()
  {
    Engine::Graphics::Allocator::destroy();
    Engine::Graphics::Device::destroy();
    Engine::Graphics::Instance::destroy();
  }

  DeviceProvider()
  {
    Engine::Graphics::Device::the();
    Engine::Graphics::Allocator::construct();
  }

  auto get_device() const -> VkDevice
  {
    return Engine::Graphics::Device::the().device();
  }

  auto get_queue_type() const -> Engine::Graphics::QueueType
  {
    return Engine::Graphics::QueueType::Graphics;
  }
};

TEST(ThreadPoolTest, ThreadPoolConstructor)
{
  using namespace Engine::Graphics;
  ASSERT_TRUE(true) << "This test is useful.";

  auto device_provider = std::make_unique<DeviceProvider>();
  ED::ThreadPool thread_pool(*device_provider);

  Engine::Core::DataBuffer buffer{ 100 * 100 * sizeof(Engine::Core::u32) };
  std::array<Engine::Core::u32, 100 * 100> data{};
  for (auto& d : data) {
    d = 0xFFFFFFFF;
  }
  buffer.write(std::span{ data });

  auto loaded = TextureGenerator::simplex_noise(100, 100);

  std::vector<Engine::Core::Ref<Engine::Graphics::Image>> images;
  std::vector<std::future<Engine::Core::Ref<Engine::Graphics::Image>>> futures;

  static constexpr auto count = 20;
  for (const auto i : std::views::iota(0, count)) {
    auto& f = futures.emplace_back();
    f = thread_pool.enqueue_command_buffer_task(
      [&images, loaded](auto& cmd_buffer) {
        auto copy = Image::copy_image(*loaded, cmd_buffer);
        return copy;
      });
  }

  for (auto& f : futures) {
    f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds{ 1 });
  }

  for (auto& f : futures) {
    images.emplace_back(f.get());
  }

  ASSERT_EQ(images.size(), count);
  for (const auto& image : images) {
    ASSERT_EQ(image->get_mip_levels(), 1);
  }
}