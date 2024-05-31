#pragma once

#include "sparks/asset_manager/asset_manager_utils.h"
#include "sparks/asset_manager/mesh_asset.h"
#include "sparks/asset_manager/texture_asset.h"

namespace sparks {
class AssetManager {
 public:
  AssetManager(vulkan::Core *core, uint32_t max_textures, uint32_t max_meshes);

  ~AssetManager();

  int LoadTexture(const Texture &texture, std::string name = "Unnamed Texture");

  int LoadMesh(const Mesh &mesh, std::string name = "Unnamed Mesh");

  TextureAsset *GetTexture(uint32_t id);

  MeshAsset *GetMesh(uint32_t id);

  uint32_t GetTextureBindingId(uint32_t id);

  uint32_t GetMeshBindingId(uint32_t id);

  void DestroyTexture(uint32_t id);

  void DestroyMesh(uint32_t id);

  void ImGui();

  vulkan::Core *Core() {
    return core_;
  }

  std::set<uint32_t> GetTextureIds();

  std::set<uint32_t> GetMeshIds();

  vulkan::DescriptorSetLayout *DescriptorSetLayout() {
    return descriptor_set_layout_.get();
  }

  VkDescriptorSet DescriptorSet(uint32_t frame_id) {
    return descriptor_sets_[frame_id]->Handle();
  }

  void Update(uint32_t frame_id);

 private:
  void CreateDefaultAssets();
  void CreateDescriptorObjects();

  void DestroyDefaultAssets();
  void DestroyDescriptorObjects();

  void UpdateMeshDataBindings(uint32_t frame_id);
  void UpdateTextureBindings(uint32_t frame_id);

  vulkan::Core *core_;
  uint32_t next_mesh_id_{};
  uint32_t next_texture_id_{};
  std::map<uint32_t, std::pair<uint32_t, std::unique_ptr<TextureAsset>>>
      textures_;
  std::map<uint32_t, std::pair<uint32_t, std::unique_ptr<MeshAsset>>> meshes_;

  std::unique_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;

  std::unique_ptr<vulkan::Sampler> linear_sampler_;
  std::unique_ptr<vulkan::Sampler> nearest_sampler_;

  std::vector<uint32_t> last_frame_bound_texture_num_;
  std::vector<uint32_t> last_frame_bound_mesh_num_;

  uint32_t max_textures_{};
  uint32_t max_meshes_{};
};
}  // namespace sparks
