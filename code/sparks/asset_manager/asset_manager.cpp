#include "sparks/asset_manager/asset_manager.h"

#include <utility>

namespace sparks {
AssetManager::AssetManager(vulkan::Core *core) : core_(core) {
  Texture white{1, 1, glm::vec4{1.0f}};
  LoadTexture(white, "Pure White");
  Mesh mesh;
  mesh.LoadObjFile(FindAssetsFile("mesh/cube.obj"));
  LoadMesh(mesh, "Cube");
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

  textures_[next_texture_id_] =
      std::make_unique<TextureAsset>(std::move(texture_asset));

  return next_texture_id_++;
}

int AssetManager::LoadMesh(const Mesh &mesh, std::string name) {
  MeshAsset mesh_asset;
  mesh_asset.name_ = std::move(name);
  if (core_->CreateStaticBuffer<Vertex>(
          mesh.Vertices().size(),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          &mesh_asset.vertex_buffer_) != VK_SUCCESS) {
    return -1;
  }

  if (core_->CreateStaticBuffer<uint32_t>(
          mesh.Indices().size(),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          &mesh_asset.index_buffer_) != VK_SUCCESS) {
    return -1;
  }

  mesh_asset.vertex_buffer_->UploadContents(mesh.Vertices().data(),
                                            mesh.Vertices().size());
  mesh_asset.index_buffer_->UploadContents(mesh.Indices().data(),
                                           mesh.Indices().size());

  //  std::cout << "Vertices: \n";
  //  for (const auto &vertex : mesh.Vertices()) {
  //    std::cout << vertex << std::endl;
  //  }
  //  std::cout << "Indices: \n[";
  //
  //  for (const auto &index : mesh.Indices()) {
  //    std::cout << index << ", ";
  //  }
  //  std::cout << "]" << std::endl;

  //  if (core_->CreateBottomLevelAccelerationStructure(
  //          mesh_asset.vertex_buffer_->GetBuffer(),
  //          mesh_asset.index_buffer_->GetBuffer(),
  //          static_cast<VkDeviceSize>(sizeof(Vertex)),
  //          &mesh_asset.blas_) != VK_SUCCESS) {
  //    LogError("Create BLAS failed.");
  //    return -1;
  //  }

  meshes_[next_mesh_id_] = std::make_unique<MeshAsset>(std::move(mesh_asset));

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
      ImGui::Text(texture->name_.c_str());
    }

    ImGui::SeparatorText("Meshes");
    for (const auto &[id, mesh] : meshes_) {
      ImGui::Text(mesh->name_.c_str());
    }
  }
  ImGui::End();
}

TextureAsset *AssetManager::GetTexture(uint32_t id) {
  if (textures_.find(id) != textures_.end()) {
    return textures_[id].get();
  }
  return textures_[0].get();
}

MeshAsset *AssetManager::GetMesh(uint32_t id) {
  if (meshes_.find(id) != meshes_.end()) {
    return meshes_[id].get();
  }
  return meshes_[0].get();
}

}  // namespace sparks
