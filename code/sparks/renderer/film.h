#pragma once
#include "sparks/renderer/renderer_utils.h"

namespace sparks {

constexpr VkFormat kAlbedoFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat kPositionFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat kNormalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat kIntensityFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

struct Film {
  std::unique_ptr<vulkan::Image> albedo_image{};
  std::unique_ptr<vulkan::Image> position_image{};
  std::unique_ptr<vulkan::Image> normal_image{};
  std::unique_ptr<vulkan::Image> intensity_image{};
  std::unique_ptr<vulkan::Image> depth_image{};
  std::unique_ptr<vulkan::Framebuffer> framebuffer{};
  Renderer *renderer{};

  void Resize(uint32_t width, uint32_t height);
};
}  // namespace sparks
