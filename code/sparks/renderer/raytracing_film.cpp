#include "sparks/renderer/raytracing_film.h"

#include "sparks/renderer/renderer.h"

namespace sparks {

void RayTracingFilm::Resize(uint32_t width, uint32_t height) {
  VkExtent2D extent{width, height};
  result_image->Resize(extent);
  accumulated_weight_image->Resize(extent);
  accumulated_radiance_image->Resize(extent);
  descriptor_set->BindStorageImage(0, result_image.get());
  descriptor_set->BindStorageImage(1, accumulated_radiance_image.get());
  descriptor_set->BindStorageImage(2, accumulated_weight_image.get());

  renderer->Core()->SingleTimeCommands([&](VkCommandBuffer cmd_buffer) {
    vulkan::TransitImageLayout(
        cmd_buffer, result_image->Handle(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    vulkan::TransitImageLayout(
        cmd_buffer, accumulated_radiance_image->Handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    vulkan::TransitImageLayout(
        cmd_buffer, accumulated_weight_image->Handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
  });
}
}  // namespace sparks
