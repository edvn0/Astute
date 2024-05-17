#include "thread_pool/CommandBufferDispatcher.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <graphics/Allocator.hpp>
#include <graphics/Device.hpp>
#include <graphics/Image.hpp>
#include <graphics/Instance.hpp>
#include <graphics/TextureGenerator.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <ranges>
#include <span>
#include <sstream>

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

class CommandBufferDispatcherTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    device_provider = std::make_unique<DeviceProvider>();
  }

  void TearDown() override { device_provider.reset(); }

  std::unique_ptr<DeviceProvider> device_provider;
};

TEST_F(CommandBufferDispatcherTests, CommandBufferDispatcher_BasicTest)
{
  using namespace Engine::Graphics;
  using namespace Engine;
  auto loaded = TextureGenerator::simplex_noise(100, 100);

  std::vector<Core::Ref<Image>> copies;
  ED::CommandBufferDispatcher dispatcher;

  static auto submit = [&]() {
    dispatcher.dispatch([loaded, &copies](auto* secondary_buffer) {
      copies.push_back(loaded->copy_image(*loaded, *secondary_buffer));
    });
  };

  for (int i = 0; i < 10; i++) {
    submit();
  }

  dispatcher.execute();

  for (const auto& copy : copies) {
    auto result =
      copy->write_to_file(std::format("test_output/{}.png", copy->hash()));
    ASSERT_TRUE(result) << "Failed to write image to file.";
  }

  for (const auto& copy : copies) {
    auto result =
      std::filesystem::remove(std::format("test_output/{}.png", copy->hash()));
    ASSERT_TRUE(result) << "Failed to remove image from file.";
  }
}
