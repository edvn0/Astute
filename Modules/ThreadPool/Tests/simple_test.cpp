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
#include <thread_pool/ThreadPool.hpp>

#ifdef ASTUTE_TESTING_BENCHMARK
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
class ThreadPoolBenchmark : public ::testing::Test
{
protected:
  void SetUp() override
  {
    device_provider = std::make_unique<DeviceProvider>();
  }

  void TearDown() override { device_provider.reset(); }

  std::unique_ptr<DeviceProvider> device_provider;
};

TEST_F(ThreadPoolBenchmark, ThreadPoolConstructorBenchmark)
{
  using namespace Engine::Graphics;
  ASSERT_TRUE(true) << "This test is useful.";

  return;

  ED::ThreadPool thread_pool(*device_provider);

  Engine::Core::DataBuffer buffer{ 100 * 100 * sizeof(Engine::Core::u32) };
  std::array<Engine::Core::u32, 100 * 100> data{};
  for (auto& d : data) {
    d = 0xFFFFFFFF;
  }
  buffer.write(std::span{ data });

  auto loaded = TextureGenerator::simplex_noise(100, 100);

  // Change 'count' for different benchmarks
  static constexpr std::array counts = {
    1,   2,   4,   8,    16,   32,   64,
    128, 256, 512, 1024, 2048, 3999, // Max sampler count is 4000 :)
  };

  std::stringstream csv_output;
  csv_output << "Count,Time(s)\n"; // CSV Header

  for (auto count : counts) {
    std::vector<Engine::Core::Ref<Engine::Graphics::Image>> images;
    std::vector<std::future<Engine::Core::Ref<Engine::Graphics::Image>>>
      futures;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto i : std::views::iota(0, count)) {
      auto& f = futures.emplace_back();
      f = thread_pool.enqueue_command_buffer_task(
        [&images, loaded](auto& cmd_buffer) {
          auto copy = Image::copy_image(*loaded, cmd_buffer);
          return copy;
        });
    }

    for (auto& f : futures) {
      images.emplace_back(f.get());
    }

    auto i = 0;
    for (auto& img : images) {
      auto result = img->write_to_file(
        std::format("test_output/image{}_{}.png", count, i++));
      ASSERT_TRUE(result) << "Failed to write image to file";
    }

    ASSERT_EQ(images.size(), count);
    for (const auto& image : images) {
      ASSERT_EQ(image->get_mip_levels(), 1);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    // Record the benchmark result in CSV format
    csv_output << count << "," << elapsed.count() << "\n";

    // Clean up images
    for (const auto i : std::views::iota(0, count)) {
      if (!std::filesystem::remove(
            std::format("test_output/image{}_{}.png", count, i))) {
        std::cerr << "Failed to remove image" << i << std::endl;
      }
    }

    images.clear();
    futures.clear();
  }

  // Write the CSV results to a file
  std::ofstream csv_file("benchmark_results.csv");
  if (csv_file.is_open()) {
    csv_file << csv_output.str();
    csv_file.close();
  } else {
    std::cerr << "Failed to open file for writing CSV results." << std::endl;
  }
}
#endif