#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Bloom.hpp"

#include "core/Scene.hpp"
#include "logging/Logger.hpp"

#include "core/Application.hpp"
#include "graphics/ComputePipeline.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

#include <imgui.h>

namespace Engine::Graphics {

auto
BloomRenderPass::construct_impl() -> void
{
  auto&& [_, shader, light_culling_pipeline, light_culling_material] =
    get_data();
  shader = Shader::compile_compute_scoped("Assets/shaders/bloom.comp");

  light_culling_pipeline =
    Core::make_scope<ComputePipeline>(ComputePipeline::Configuration{
      .shader = shader.get(),
    });
  light_culling_material = Core::make_scope<Material>(Material::Configuration{
    .shader = shader.get(),
  });
  light_culling_material->set(
    "predepth_map",
    get_renderer().get_render_pass("Predepth").get_depth_attachment());

  for (auto& bloom_img : bloom_chain) {
    bloom_img = Core::make_ref<Image>(ImageConfiguration{
      .width = get_renderer().get_size().width,
      .height = get_renderer().get_size().height,
      .mip_levels = compute_mips_from_width_height(
        get_renderer().get_size().width, get_renderer().get_size().height),
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .is_transfer = true,
      .layout = VK_IMAGE_LAYOUT_GENERAL,
      .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    });
    Device::the().execute_immediate([&](auto* buf) {
      transition_image_layout(buf,
                              bloom_img->image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              bloom_img->get_aspect_flags(),
                              bloom_img->get_mip_levels());
      bloom_img->generate_mips(buf);
      transition_image_layout(buf,
                              bloom_img->image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_GENERAL,
                              bloom_img->get_aspect_flags(),
                              bloom_img->get_mip_levels());
    });
  }
}

auto
BloomRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();

  auto workgroup_size =
    get_current_settings<BloomSettings>()->bloom_workgroup_size;

  auto& [_, shader, pipeline, light_culling_material] = get_data();

  struct BloomComputePushConstants
  {
    glm::vec4 Params;
    float LOD = 0.0f;
    int Mode = 0;
  } bloomComputePushConstants;
  auto* casted_settings = static_cast<BloomSettings*>(get_settings());
  bloomComputePushConstants.Params = {
    casted_settings->Threshold,
    casted_settings->Threshold - casted_settings->Knee,
    casted_settings->Knee * 2.0f,
    0.25f / casted_settings->Knee,
  };
  bloomComputePushConstants.Mode = 0;

  const auto& inputImage =
    get_renderer().get_render_pass("Deferred").get_colour_attachment(0);

  // m_GPUTimeQueries.BloomComputePassQuery =
  //   m_CommandBuffer->BeginTimestampQuery();

  VkDevice device = Device::the().device();

  auto descriptorImageInfo = bloom_chain.at(0)->get_descriptor_info();
  descriptorImageInfo.imageView = bloom_chain.at(0)->get_mip_image_view(0);

  std::array<VkWriteDescriptorSet, 3> write_descriptors;

  auto* descriptorSetLayout = shader->get_descriptor_set_layouts().at(0);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  // Output image
  VkDescriptorSet descriptorSet =
    shader->allocate_descriptor_set(0).descriptor_sets.at(0);
  write_descriptors[0] = *shader->get_descriptor_set("output_image", 0);
  write_descriptors[0].dstSet =
    descriptorSet; // Should this be set inside the shader?
  write_descriptors[0].pImageInfo = &descriptorImageInfo;

  // Input image
  write_descriptors[1] = *shader->get_descriptor_set("input_texture");
  write_descriptors[1].dstSet =
    descriptorSet; // Should this be set inside the shader?
  write_descriptors[1].pImageInfo = &inputImage->get_descriptor_info();

  write_descriptors[2] = *shader->get_descriptor_set("input_bloom_texture");
  write_descriptors[2].dstSet =
    descriptorSet; // Should this be set inside the shader?
  write_descriptors[2].pImageInfo = &inputImage->get_descriptor_info();

  vkUpdateDescriptorSets(device,
                         (uint32_t)write_descriptors.size(),
                         write_descriptors.data(),
                         0,
                         nullptr);

  uint32_t workGroupsX = bloom_chain[0]->configuration.width / workgroup_size;
  uint32_t workGroupsY = bloom_chain[0]->configuration.height / workgroup_size;

  // Renderer::RT_BeginGPUPerfMarker(commandBuffer, "Bloom-Prefilter");
  vkCmdPushConstants(command_buffer.get_command_buffer(),
                     pipeline->get_layout(),
                     VK_SHADER_STAGE_ALL,
                     0,
                     sizeof(bloomComputePushConstants),
                     &bloomComputePushConstants);
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          pipeline->get_bind_point(),
                          pipeline->get_layout(),
                          0,
                          1,
                          &descriptorSet,
                          0,
                          nullptr);
  vkCmdDispatch(
    command_buffer.get_command_buffer(), workGroupsX, workGroupsY, 1);
  // Renderer::RT_EndGPUPerfMarker(commandBuffer);

  {
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = bloom_chain[0]->image;
    imageMemoryBarrier.subresourceRange = {
      VK_IMAGE_ASPECT_COLOR_BIT, 0, bloom_chain[0]->get_mip_levels(), 0, 1
    };
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command_buffer.get_command_buffer(),
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &imageMemoryBarrier);
  }

  bloomComputePushConstants.Mode = 1;

  uint32_t mips = bloom_chain[0]->get_mip_levels() - 2;
  // Renderer::RT_BeginGPUPerfMarker(commandBuffer, "Bloom-DownSample");
  for (uint32_t i = 1; i < mips; i++) {
    auto [mipWidth, mipHeight] = bloom_chain[0]->get_mip_size(i);
    workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workgroup_size);
    workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workgroup_size);

    {
      // Output image
      descriptorImageInfo.imageView = bloom_chain[1]->get_mip_image_view(i);

      descriptorSet = shader->allocate_descriptor_set(0).descriptor_sets.at(0);
      write_descriptors[0] = *shader->get_descriptor_set("output_image");
      write_descriptors[0].dstSet =
        descriptorSet; // Should this be set inside the shader?
      write_descriptors[0].pImageInfo = &descriptorImageInfo;

      // Input image
      write_descriptors[1] = *shader->get_descriptor_set("input_texture");
      write_descriptors[1].dstSet =
        descriptorSet; // Should this be set inside the shader?
      const auto& descriptor = bloom_chain[0]->get_descriptor_info();
      // descriptor.sampler = samplerClamp;
      write_descriptors[1].pImageInfo = &descriptor;

      write_descriptors[2] = *shader->get_descriptor_set("input_bloom_texture");
      write_descriptors[2].dstSet =
        descriptorSet; // Should this be set inside the shader?
      write_descriptors[2].pImageInfo = &inputImage->get_descriptor_info();

      vkUpdateDescriptorSets(device,
                             (uint32_t)write_descriptors.size(),
                             write_descriptors.data(),
                             0,
                             nullptr);

      bloomComputePushConstants.LOD = i - 1.0f;
      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         pipeline->get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         sizeof(bloomComputePushConstants),
                         &bloomComputePushConstants);
      vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                              pipeline->get_bind_point(),
                              pipeline->get_layout(),
                              0,
                              1,
                              &descriptorSet,
                              0,
                              nullptr);
      vkCmdDispatch(
        command_buffer.get_command_buffer(), workGroupsX, workGroupsY, 1);
    }

    {
      VkImageMemoryBarrier imageMemoryBarrier = {};
      imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.image = bloom_chain[1]->image;
      imageMemoryBarrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, bloom_chain[1]->get_mip_levels(), 0, 1
      };
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(command_buffer.get_command_buffer(),
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &imageMemoryBarrier);
    }

    {
      descriptorImageInfo.imageView = bloom_chain[0]->get_mip_image_view(i);

      // Output image
      descriptorSet = shader->allocate_descriptor_set(0).descriptor_sets.at(0);
      write_descriptors[0] = *shader->get_descriptor_set("output_image");
      write_descriptors[0].dstSet =
        descriptorSet; // Should this be set inside the shader?
      write_descriptors[0].pImageInfo = &descriptorImageInfo;

      // Input image
      write_descriptors[1] = *shader->get_descriptor_set("input_texture");
      write_descriptors[1].dstSet =
        descriptorSet; // Should this be set inside the shader?
      const auto& descriptor = bloom_chain[1]->get_descriptor_info();
      // descriptor.sampler = samplerClamp;
      write_descriptors[1].pImageInfo = &descriptor;

      write_descriptors[2] = *shader->get_descriptor_set("input_bloom_texture");
      write_descriptors[2].dstSet =
        descriptorSet; // Should this be set inside the shader?
      write_descriptors[2].pImageInfo = &inputImage->get_descriptor_info();

      vkUpdateDescriptorSets(device,
                             (uint32_t)write_descriptors.size(),
                             write_descriptors.data(),
                             0,
                             nullptr);

      bloomComputePushConstants.LOD = (float)i;
      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         pipeline->get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         sizeof(bloomComputePushConstants),
                         &bloomComputePushConstants);
      vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                              pipeline->get_bind_point(),
                              pipeline->get_layout(),
                              0,
                              1,
                              &descriptorSet,
                              0,
                              nullptr);
      vkCmdDispatch(
        command_buffer.get_command_buffer(), workGroupsX, workGroupsY, 1);
    }

    {
      VkImageMemoryBarrier imageMemoryBarrier = {};
      imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.image = bloom_chain[0]->image;
      imageMemoryBarrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, bloom_chain[0]->get_mip_levels(), 0, 1
      };
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(command_buffer.get_command_buffer(),
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &imageMemoryBarrier);
    }
  }
  // Renderer::RT_EndGPUPerfMarker(commandBuffer);

  // Renderer::RT_BeginGPUPerfMarker(commandBuffer, "Bloom-FirstUpsamle");
  bloomComputePushConstants.Mode = 2;
  workGroupsX *= 2;
  workGroupsY *= 2;

  // Output image
  descriptorSet = shader->allocate_descriptor_set(0).descriptor_sets.at(0);
  descriptorImageInfo.imageView = bloom_chain[2]->get_mip_image_view(mips - 2);

  write_descriptors[0] = *shader->get_descriptor_set("output_image");
  write_descriptors[0].dstSet =
    descriptorSet; // Should this be set inside the shader?
  write_descriptors[0].pImageInfo = &descriptorImageInfo;

  // Input image
  write_descriptors[1] = *shader->get_descriptor_set("input_texture");
  write_descriptors[1].dstSet =
    descriptorSet; // Should this be set inside the shader?
  write_descriptors[1].pImageInfo = &bloom_chain[0]->get_descriptor_info();

  write_descriptors[2] = *shader->get_descriptor_set("input_bloom_texture");
  write_descriptors[2].dstSet =
    descriptorSet; // Should this be set inside the shader?
  write_descriptors[2].pImageInfo = &inputImage->get_descriptor_info();

  vkUpdateDescriptorSets(device,
                         (uint32_t)write_descriptors.size(),
                         write_descriptors.data(),
                         0,
                         nullptr);

  auto [mipWidth, mipHeight] = bloom_chain[2]->get_mip_size(mips - 2);
  workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workgroup_size);
  workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workgroup_size);
  bloomComputePushConstants.LOD--;
  vkCmdPushConstants(command_buffer.get_command_buffer(),
                     pipeline->get_layout(),
                     VK_SHADER_STAGE_ALL,
                     0,
                     sizeof(bloomComputePushConstants),
                     &bloomComputePushConstants);
  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          pipeline->get_bind_point(),
                          pipeline->get_layout(),
                          0,
                          1,
                          &descriptorSet,
                          0,
                          nullptr);
  vkCmdDispatch(
    command_buffer.get_command_buffer(), workGroupsX, workGroupsY, 1);

  {
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = bloom_chain[2]->image;
    imageMemoryBarrier.subresourceRange = {
      VK_IMAGE_ASPECT_COLOR_BIT, 0, bloom_chain[2]->get_mip_levels(), 0, 1
    };
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command_buffer.get_command_buffer(),
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &imageMemoryBarrier);
  }

  // Renderer::RT_EndGPUPerfMarker(commandBuffer);

  // Renderer::RT_BeginGPUPerfMarker(commandBuffer, "Bloom-Upsample");
  bloomComputePushConstants.Mode = 3;

  // Upsample
  for (int32_t mip = mips - 3; mip >= 0; mip--) {
    auto&& [current_mip_width, current_mip_height] =
      bloom_chain[2]->get_mip_size(mip);
    workGroupsX =
      (uint32_t)glm::ceil((float)current_mip_width / (float)workgroup_size);
    workGroupsY =
      (uint32_t)glm::ceil((float)current_mip_height / (float)workgroup_size);

    // Output image
    descriptorImageInfo.imageView = bloom_chain[2]->get_mip_image_view(mip);
    auto current_descriptor_set =
      shader->allocate_descriptor_set(0).descriptor_sets.at(0);
    write_descriptors[0] = *shader->get_descriptor_set("output_image");
    write_descriptors[0].dstSet =
      current_descriptor_set; // Should this be set inside the shader?
    write_descriptors[0].pImageInfo = &descriptorImageInfo;

    // Input image
    write_descriptors[1] = *shader->get_descriptor_set("input_texture");
    write_descriptors[1].dstSet =
      current_descriptor_set; // Should this be set inside the shader?
    write_descriptors[1].pImageInfo = &bloom_chain[0]->get_descriptor_info();

    write_descriptors[2] = *shader->get_descriptor_set("input_bloom_texture");
    write_descriptors[2].dstSet =
      current_descriptor_set; // Should this be set inside the shader?
    write_descriptors[2].pImageInfo = &bloom_chain[2]->get_descriptor_info();

    vkUpdateDescriptorSets(device,
                           (uint32_t)write_descriptors.size(),
                           write_descriptors.data(),
                           0,
                           nullptr);

    bloomComputePushConstants.LOD = (float)mip;
    vkCmdPushConstants(command_buffer.get_command_buffer(),
                       pipeline->get_layout(),
                       VK_SHADER_STAGE_ALL,
                       0,
                       sizeof(bloomComputePushConstants),
                       &bloomComputePushConstants);
    vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                            pipeline->get_bind_point(),
                            pipeline->get_layout(),
                            0,
                            1,
                            &current_descriptor_set,
                            0,
                            nullptr);
    vkCmdDispatch(
      command_buffer.get_command_buffer(), workGroupsX, workGroupsY, 1);

    {
      VkImageMemoryBarrier imageMemoryBarrier = {};
      imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.image = bloom_chain[2]->image;
      imageMemoryBarrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, bloom_chain[2]->get_mip_levels(), 0, 1
      };
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(command_buffer.get_command_buffer(),
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &imageMemoryBarrier);
    }
  }
  // Renderer::RT_EndGPUPerfMarker(commandBuffer);

  // pipeline->End();
  //  m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.BloomComputePassQuery);
}

auto
BloomRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto& [___, __, pipe, _] = get_data();
  pipe->on_resize(ext);

  {
    auto* current_settings = get_current_settings<BloomSettings>();
    glm::uvec2 bloomSize = (glm::uvec2(ext.width, ext.height) + 1u) / 2u;
    bloomSize += current_settings->bloom_workgroup_size -
                 bloomSize % current_settings->bloom_workgroup_size;
    bloom_chain[0]->configuration.width = ext.width;
    bloom_chain[0]->configuration.height = ext.height;
    bloom_chain[0]->invalidate();
    bloom_chain[1]->configuration.width = ext.width;
    bloom_chain[1]->configuration.height = ext.height;
    bloom_chain[1]->invalidate();
    bloom_chain[2]->configuration.width = ext.width;
    bloom_chain[2]->configuration.height = ext.height;
    bloom_chain[2]->invalidate();
  }
}

auto
BloomRenderPass::BloomSettings::expose_to_ui(Material&) -> void
{
  ImGui::Text("Bloom Settings");
  ImGui::DragFloat("Bloom Threshold", &Threshold, 0.01F, 0.01F, 3.0F);
  ImGui::DragFloat("Bloom Knee", &Knee, 0.01F, 0.01F, 3.0F);
}

auto
BloomRenderPass::BloomSettings::apply_to_material(Material&) -> void
{
}

} // namespace Engine::Graphics
