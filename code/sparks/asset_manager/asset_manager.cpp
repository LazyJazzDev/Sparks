#include "sparks/asset_manager/asset_manager.h"

#include <utility>

namespace sparks {
AssetManager::AssetManager(vulkan::Core *core,
                           uint32_t max_textures,
                           uint32_t max_meshes)
    : core_(core), max_textures_(max_textures), max_meshes_(max_meshes) {
  CreateDescriptorObjects();
  CreateDefaultAssets();
}

AssetManager::~AssetManager() {
  DestroyDefaultAssets();
  DestroyDescriptorObjects();
}

void AssetManager::CreateDefaultAssets() {
  Texture white{1, 1, glm::vec4{1.0f}};
  LoadTexture(white, "Pure White");
  Mesh mesh;
  mesh.LoadObjFile(FindAssetsFile("mesh/cube.obj"));
  LoadMesh(mesh, "Cube");
}

void AssetManager::DestroyDefaultAssets() {
  textures_.clear();
  meshes_.clear();
}

void AssetManager::CreateDescriptorObjects() {
  core_->Device()->CreateSampler(
      VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE,
      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, VK_SAMPLER_MIPMAP_MODE_LINEAR,
      &linear_sampler_);

  core_->Device()->CreateSampler(
      VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE,
      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, VK_SAMPLER_MIPMAP_MODE_NEAREST,
      &nearest_sampler_);

  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_meshes_,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
       {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_meshes_,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
       {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, max_textures_,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
       {3, VK_DESCRIPTOR_TYPE_SAMPLER, 2, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        nullptr}},
      &descriptor_set_layout_);

  vulkan::DescriptorPoolSize pool_size =
      descriptor_set_layout_->GetPoolSize() * core_->MaxFramesInFlight();
  core_->Device()->CreateDescriptorPool(pool_size, core_->MaxFramesInFlight(),
                                        &descriptor_pool_);
  descriptor_sets_.resize(core_->MaxFramesInFlight());
  for (size_t frame_id = 0; frame_id < core_->MaxFramesInFlight(); frame_id++) {
    auto &descriptor_set = descriptor_sets_[frame_id];
    descriptor_pool_->AllocateDescriptorSet(descriptor_set_layout_->Handle(),
                                            &descriptor_set);
    core_->Device()->NameObject(
        descriptor_set->Handle(),
        fmt::format("Asset Manager Descriptor Set [{}]", frame_id));
  }

  for (size_t frame_id = 0; frame_id < core_->MaxFramesInFlight(); frame_id++) {
    auto &descriptor_set = descriptor_sets_[frame_id];
    descriptor_set->BindSamplers(
        3, {linear_sampler_->Handle(), nearest_sampler_->Handle()});
  }

  last_frame_bound_mesh_num_ =
      std::vector<uint32_t>(core_->MaxFramesInFlight(), max_meshes_);
  last_frame_bound_texture_num_ =
      std::vector<uint32_t>(core_->MaxFramesInFlight(), max_textures_);
}

void AssetManager::DestroyDescriptorObjects() {
  descriptor_sets_.clear();
  descriptor_pool_.reset();
  descriptor_set_layout_.reset();
  linear_sampler_.reset();
  nearest_sampler_.reset();
}

int AssetManager::LoadTexture(const Texture &texture, std::string name) {
  TextureAsset texture_asset;
  texture_asset.name_ = std::move(name);
  if (core_->Device()->CreateImage(
          VK_FORMAT_R32G32B32A32_SFLOAT,
          VkExtent2D{texture.Width(), texture.Height()},
          &texture_asset.image_) != VK_SUCCESS) {
    return -1;
  }

  vulkan::UploadImage(core_->GraphicsQueue(), core_->GraphicsCommandPool(),
                      texture_asset.image_.get(), texture.Data(),
                      texture.Width() * texture.Height() * sizeof(glm::vec4));

  uint32_t binding_texture_id = textures_.size();

  textures_[next_texture_id_] = {
      binding_texture_id,
      std::make_unique<TextureAsset>(std::move(texture_asset))};

  return next_texture_id_++;
}

int AssetManager::LoadMesh(const Mesh &mesh, std::string name) {
  MeshAsset mesh_asset;
  mesh_asset.name_ = std::move(name);
  if (core_->CreateStaticBuffer<Vertex>(
          mesh.Vertices().size(),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          &mesh_asset.vertex_buffer_) != VK_SUCCESS) {
    return -1;
  }

  if (core_->CreateStaticBuffer<uint32_t>(
          mesh.Indices().size(),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          &mesh_asset.index_buffer_) != VK_SUCCESS) {
    return -1;
  }

  mesh_asset.vertex_buffer_->UploadContents(mesh.Vertices().data(),
                                            mesh.Vertices().size());
  mesh_asset.index_buffer_->UploadContents(mesh.Indices().data(),
                                           mesh.Indices().size());

  if (core_->CreateBottomLevelAccelerationStructure(
          mesh_asset.vertex_buffer_->GetBuffer(),
          mesh_asset.index_buffer_->GetBuffer(), sizeof(Vertex),
          &mesh_asset.blas_) != VK_SUCCESS) {
    return -1;
  }

  uint32_t binding_mesh_id = meshes_.size();
  meshes_[next_mesh_id_] = {binding_mesh_id,
                            std::make_unique<MeshAsset>(std::move(mesh_asset))};

  return next_mesh_id_++;
}

void AssetManager::DestroyTexture(uint32_t id) {
  textures_.erase(id);
}

void AssetManager::DestroyMesh(uint32_t id) {
  meshes_.erase(id);
}

void AssetManager::ImGui() {
  if (ImGui::Begin("Asset Manager")) {
    ImGui::SeparatorText("Textures");
    for (const auto &[id, texture] : textures_) {
      ImGui::Text("%s", texture.second->name_.c_str());
    }

    ImGui::SeparatorText("Meshes");
    for (const auto &[id, mesh] : meshes_) {
      ImGui::Text("%s", mesh.second->name_.c_str());
    }
  }
  ImGui::End();
}

TextureAsset *AssetManager::GetTexture(uint32_t id) {
  if (textures_.find(id) != textures_.end()) {
    return textures_[id].second.get();
  }
  return textures_[0].second.get();
}

MeshAsset *AssetManager::GetMesh(uint32_t id) {
  if (meshes_.find(id) != meshes_.end()) {
    return meshes_[id].second.get();
  }
  return meshes_[0].second.get();
}

uint32_t AssetManager::GetTextureBindingId(uint32_t id) {
  return textures_[id].first;
}

uint32_t AssetManager::GetMeshBindingId(uint32_t id) {
  return meshes_[id].first;
}

std::set<uint32_t> AssetManager::GetTextureIds() {
  std::set<uint32_t> ids;
  uint32_t binding_texture_ids = 0;
  for (auto &[id, texture] : textures_) {
    ids.insert(id);
    texture.first = binding_texture_ids++;
  }
  return ids;
}

std::set<uint32_t> AssetManager::GetMeshIds() {
  std::set<uint32_t> ids;
  uint32_t binding_mesh_ids = 0;
  for (auto &[id, mesh] : meshes_) {
    ids.insert(id);
    mesh.first = binding_mesh_ids++;
  }
  return ids;
}

void AssetManager::UpdateMeshDataBindings(uint32_t frame_id) {
  auto &descriptor_set = descriptor_sets_[frame_id];

  std::vector<const vulkan::Buffer *> vertex_buffers;
  std::vector<const vulkan::Buffer *> index_buffers;
  for (auto mesh_id : GetMeshIds()) {
    auto mesh = GetMesh(mesh_id);
    vertex_buffers.push_back(mesh->vertex_buffer_->GetBuffer(frame_id));
    index_buffers.push_back(mesh->index_buffer_->GetBuffer(frame_id));
  }

  uint32_t last_frame_bound_mesh_num = last_frame_bound_mesh_num_[frame_id];
  last_frame_bound_mesh_num_[frame_id] = vertex_buffers.size();
  if (last_frame_bound_mesh_num != vertex_buffers.size()) {
    while (vertex_buffers.size() < last_frame_bound_mesh_num ||
           index_buffers.size() < last_frame_bound_mesh_num) {
      auto mesh = GetMesh(0);
      vertex_buffers.push_back(mesh->vertex_buffer_->GetBuffer(frame_id));
      index_buffers.push_back(mesh->index_buffer_->GetBuffer(frame_id));
    }
  }

  descriptor_set->BindStorageBuffers(0, vertex_buffers);
  descriptor_set->BindStorageBuffers(1, index_buffers);
}

void AssetManager::UpdateTextureBindings(uint32_t frame_id) {
  std::vector<const vulkan::Image *> images;
  for (auto texture_id : GetTextureIds()) {
    auto texture = GetTexture(texture_id);
    images.push_back(texture->image_.get());
  }

  uint32_t last_frame_bound_texture_num =
      last_frame_bound_texture_num_[frame_id];
  last_frame_bound_texture_num_[frame_id] = images.size();
  if (last_frame_bound_texture_num != images.size()) {
    while (images.size() < last_frame_bound_texture_num) {
      auto texture = GetTexture(0);
      images.push_back(texture->image_.get());
    }
  }

  auto &descriptor_set = descriptor_sets_[frame_id];
  descriptor_set->BindSampledImages(2, images);
}

void AssetManager::Update(uint32_t frame_id) {
  UpdateMeshDataBindings(frame_id);
  UpdateTextureBindings(frame_id);
}

}  // namespace sparks
