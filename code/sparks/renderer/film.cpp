#include "sparks/renderer/film.h"

#include "sparks/renderer/renderer.h"

namespace sparks {

void Film::Resize(uint32_t width, uint32_t height) {
  VkExtent2D extent{width, height};
  albedo_image->Resize(extent);
  position_image->Resize(extent);
  normal_image->Resize(extent);
  radiance_image->Resize(extent);
  depth_image->Resize(extent);
  stencil_image->Resize(extent);
  renderer->RenderPass()->CreateFramebuffer(
      {albedo_image->ImageView(), position_image->ImageView(),
       normal_image->ImageView(), radiance_image->ImageView(),
       depth_image->ImageView(), stencil_image->ImageView()},
      VkExtent2D{width, height}, &framebuffer);

  renderer->Core()->SingleTimeCommands(
      [image = stencil_image->Handle()](VkCommandBuffer cmd_buffer) {
        vulkan::TransitImageLayout(
            cmd_buffer, image, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
      });
}

}  // namespace sparks
