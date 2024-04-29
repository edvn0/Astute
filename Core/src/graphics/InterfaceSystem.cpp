#include "pch/CorePCH.hpp"

#include "graphics/InterfaceSystem.hpp"

#include "core/Logger.hpp"
#include "core/Platform.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include <format>
#include <fstream>

// clang-format off
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
// clang-format on

namespace Engine::Graphics {

InterfaceSystem::InterfaceSystem(const Window& win)
  : window(&win)
{
  system_name = std::format("imgui_{}.ini", Core::Platform::get_system_name());

  std::array<VkDescriptorPoolSize, 11> pool_sizes = {
    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
  };

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = static_cast<Core::u32>(pool_sizes.size()) * 11UL;
  pool_info.poolSizeCount = static_cast<Core::u32>(std::size(pool_sizes));
  pool_info.pPoolSizes = pool_sizes.data();

  const auto& device = Device::the();

  vkCreateDescriptorPool(device.device(), &pool_info, nullptr, &pool);

  image_pool = Core::make_scope<Core::FrameBasedCollection<VkDescriptorPool>>();
  image_pool->for_each([&](const auto& k, auto& v) {
    vkCreateDescriptorPool(device.device(), &pool_info, nullptr, &v);
  });

  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

  ImGui::GetIO().IniFilename = system_name.c_str();
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0F;
    style.Colors[ImGuiCol_WindowBg].w = 1.0F;
  }
  style.Colors[ImGuiCol_WindowBg] =
    ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

  ImGui_ImplGlfw_InitForVulkan(const_cast<GLFWwindow*>(window->get_native()),
                               true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = Instance::the().instance();
  init_info.PhysicalDevice = device.physical();
  init_info.Device = device.device();
  init_info.Queue = device.get_queue(QueueType::Graphics);
  init_info.ColorAttachmentFormat = window->get_swapchain().get_colour_format();
  init_info.DescriptorPool = pool;
  init_info.MinImageCount = 2;
  init_info.ImageCount = window->get_swapchain().get_image_count();
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&init_info, window->get_swapchain().get_renderpass());

  auto image_count = window->get_swapchain().get_image_count();
  for (auto i = 0U; i < image_count; i++) {
    command_buffers.emplace_back(
      Device::the().create_secondary_command_buffer());
  }
}

auto
InterfaceSystem::begin_frame() -> void
{
  const auto& device = Device::the();
  vkResetDescriptorPool(device.device(), image_pool->get(), 0);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  if (font) {
    ImGui::PushFont(font);
  }
  // ImGuizmo::BeginFrame();
}

auto
InterfaceSystem::end_frame() -> void
{
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f));
  // ImGui::RenderNotifications();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(1);

  if (font) {
    ImGui::PopFont();
  }
  ImGui::Render();
  static constexpr VkClearColorValue clear_colour{
    {
      0.1F,
      0.1F,
      0.1F,
      0.1F,
    },
  };

  std::array<VkClearValue, 1> clear_values{};
  clear_values[0].color = clear_colour;

  const auto& [width, height] = window->get_swapchain().get_size();

  auto frame_index = window->get_swapchain().get_current_buffer_index();

  auto draw_command_buffer = window->get_swapchain().get_drawbuffer();

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pNext = nullptr;
  vkBeginCommandBuffer(draw_command_buffer, &begin_info);

  auto* vk_render_pass = window->get_swapchain().get_renderpass();
  auto* vk_framebuffer = window->get_swapchain().get_framebuffer();

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = vk_render_pass;
  render_pass_begin_info.renderArea.offset.x = 0;
  render_pass_begin_info.renderArea.offset.y = 0;
  render_pass_begin_info.renderArea.extent.width = width;
  render_pass_begin_info.renderArea.extent.height = height;
  render_pass_begin_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_begin_info.pClearValues = clear_values.data();
  render_pass_begin_info.framebuffer = vk_framebuffer;

  // DebugMarker::begin_region(draw_command_buffer, "Interface", { 1, 0, 0, 1
  // });

  // Device::the().reset_command_pools();

  vkCmdBeginRenderPass(draw_command_buffer,
                       &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  {
    VkCommandBufferInheritanceInfo inheritance_info = {};
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.renderPass = vk_render_pass;
    inheritance_info.framebuffer = vk_framebuffer;

    VkCommandBufferBeginInfo inherited_begin_info = {};
    inherited_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    inherited_begin_info.flags =
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    inherited_begin_info.pInheritanceInfo = &inheritance_info;

    const auto& command_buffer = command_buffers[frame_index].buffer;
    vkBeginCommandBuffer(command_buffer, &inherited_begin_info);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(height);
    viewport.height = -static_cast<float>(height);
    viewport.width = static_cast<float>(width);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    ImDrawData* main_draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(main_draw_data, command_buffer);

    vkEndCommandBuffer(command_buffer);
  }

  std::array buffer{
    command_buffers[frame_index].buffer,
  };
  vkCmdExecuteCommands(draw_command_buffer, 1, buffer.data());

  // DebugMarker::end_region(draw_command_buffer);
  vkCmdEndRenderPass(draw_command_buffer);

  vkEndCommandBuffer(draw_command_buffer);

  if (const auto& imgui_io = ImGui::GetIO();
      imgui_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
}

InterfaceSystem::~InterfaceSystem()
{
  const auto& device = Device::the();
  vkDeviceWaitIdle(device.device());

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  vkDestroyDescriptorPool(device.device(), pool, nullptr);

  image_pool->for_each([&](auto, auto& v) {
    vkDestroyDescriptorPool(device.device(), v, nullptr);
  });
}

} // namespace Core
