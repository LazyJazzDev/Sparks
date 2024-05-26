#pragma once
#include "sparks/renderer/renderer_utils.h"

namespace sparks {
struct Film {
  std::unique_ptr<vulkan::Image> albedo_image_;
  std::unique_ptr<vulkan::Image> normal_image_;
  std::unique_ptr<vulkan::Image> intensity_image_;
  std::unique_ptr<vulkan::Image> depth_image_;
  std::unique_ptr<vulkan::Framebuffer> framebuffer_;

  void Resize(uint32_t width, uint32_t height);
};
}  // namespace sparks
