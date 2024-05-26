#pragma once
#include "sparks/asset_manager/asset_manager_utils.h"

namespace sparks {
struct MeshAsset {
  std::unique_ptr<vulkan::StaticBuffer<Vertex>> vertex_buffer_;
  std::unique_ptr<vulkan::StaticBuffer<uint32_t>> index_buffer_;
  std::unique_ptr<vulkan::AccelerationStructure> blas_;
  std::string name_;
};
}  // namespace sparks
