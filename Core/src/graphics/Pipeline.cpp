#include "pch/CorePCH.hpp"

#include "graphics/Pipeline.hpp"

#include "graphics/Device.hpp"

#include "core/Verify.hpp"

namespace Engine::Graphics {

IPipeline::~IPipeline()
{
  if (pipeline_cache != nullptr) {
    vkDestroyPipelineCache(Device::the().device(), pipeline_cache, nullptr);
  }
}

void
IPipeline::create_pipeline_cache(const std::string& cache_filename)
{
  VkPipelineCacheCreateInfo pipeline_cache_info{};
  pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

  std::ifstream cache_file(cache_filename, std::ios::binary);
  std::vector<char> cache_data{};
  if (cache_file) {
    cache_data = {
      std::istreambuf_iterator<char>(cache_file),
      std::istreambuf_iterator<char>(),
    };
    pipeline_cache_info.initialDataSize = cache_data.size();
    pipeline_cache_info.pInitialData = cache_data.data();
  }

  VK_CHECK(vkCreatePipelineCache(
    Device::the().device(), &pipeline_cache_info, nullptr, &pipeline_cache));
}

void
IPipeline::save_pipeline_cache(const std::string& cache_filename)
{
  if (pipeline_cache == nullptr) {
    info("Pipeline cache for {} missing.", cache_filename);
    return;
  }

  Core::usize cache_size = 0;
  vkGetPipelineCacheData(
    Device::the().device(), pipeline_cache, &cache_size, nullptr);

  std::vector<char> cache_data(cache_size);
  vkGetPipelineCacheData(
    Device::the().device(), pipeline_cache, &cache_size, cache_data.data());

  std::ofstream cache_file(cache_filename, std::ios::binary);
  cache_file.write(cache_data.data(),
                   static_cast<std::streamsize>(cache_data.size()));
}

}
