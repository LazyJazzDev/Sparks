#include "sparks/renderer/film.h"

namespace sparks {

void Film::Resize(uint32_t width, uint32_t height) {
  VkExtent2D extent{width, height};
  albedo_image_->Resize(extent);
  normal_image_->Resize(extent);
  intensity_image_->Resize(extent);
  depth_image_->Resize(extent);
}
}  // namespace sparks
