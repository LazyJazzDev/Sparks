#pragma once
#include "sparks/asset_manager/asset_manager_utils.h"

namespace sparks {

struct MeshMetadata {
  uint32_t num_vertex;
  uint32_t num_index;
};

struct MeshAsset {
  std::unique_ptr<vulkan::StaticBuffer<Vertex>> vertex_buffer_;
  std::unique_ptr<vulkan::StaticBuffer<uint32_t>> index_buffer_;
  std::unique_ptr<vulkan::StaticBuffer<float>> area_cdf_buffer_;
  std::unique_ptr<vulkan::AccelerationStructure> blas_;
  std::string name_;
  float area_;
};
}  // namespace sparks
