#include "sparks/renderer/film.h"

#include "sparks/renderer/renderer.h"

namespace sparks {

void Film::Resize(uint32_t width, uint32_t height) {
  VkExtent2D extent{width, height};
  albedo_image->Resize(extent);
  position_image->Resize(extent);
  normal_image->Resize(extent);
  intensity_image->Resize(extent);
  depth_image->Resize(extent);
  renderer->RenderPass()->CreateFramebuffer(
      {albedo_image->ImageView(), position_image->ImageView(),
       normal_image->ImageView(), intensity_image->ImageView(),
       depth_image->ImageView()},
      VkExtent2D{width, height}, &framebuffer);
}

}  // namespace sparks
