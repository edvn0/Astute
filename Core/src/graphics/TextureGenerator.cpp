#include "pch/CorePCH.hpp"

#include "graphics/GPUBuffer.hpp"
#include "graphics/TextureGenerator.hpp"

#include <FastNoise/FastNoise.h>

namespace Engine::Graphics {

auto
TextureGenerator::simplex_noise(Core::u32 w, Core::u32 h) -> Core::Ref<Image>
{
  Core::DataBuffer buffer{ w * h * 4 };
  std::vector<Core::f32> data(w * h);

  static auto simplex_algorithm = FastNoise::New<FastNoise::Simplex>();
  static auto fractal_algorithm = FastNoise::New<FastNoise::FractalFBm>();

  fractal_algorithm->SetSource(simplex_algorithm);
  fractal_algorithm->SetOctaveCount(5);

  static auto seed = 0xdeadbeef;
  fractal_algorithm->GenUniformGrid2D(data.data(), 0, 0, w, h, 0.2F, seed);

  buffer.write(std::span{ data });

  auto image = Image::construct({
    .width = w,
    .height = h,
    .additional_name_data = "SimpleNoise",
  });
  Graphics::StagingBuffer staging_buffer{ std::move(buffer) };
  Device::the().execute_immediate([&](auto* buf) {
    transition_image_layout(buf,
                            image->image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            image->get_aspect_flags(),
                            image->get_mip_levels(),
                            0);
    copy_buffer_to_image(buf,
                         staging_buffer.get_buffer(),
                         image->image,
                         image->configuration.width,
                         image->configuration.height);
    transition_image_layout(buf,
                            image->image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            image->get_layout(),
                            image->get_aspect_flags(),
                            image->get_mip_levels(),
                            0);
  });

  return image;
}

} // namespace Engine::Graphics
