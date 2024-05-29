#pragma once

#include "sparks/asset_manager/asset_manager_utils.h"
#include "sparks/asset_manager/mesh_asset.h"
#include "sparks/asset_manager/texture_asset.h"

namespace sparks {
class AssetManager {
 public:
  AssetManager(vulkan::Core *core);

  ~AssetManager() = default;

  int LoadTexture(const Texture &texture, std::string name = "Unnamed Texture");

  int LoadMesh(const Mesh &mesh, std::string name = "Unnamed Mesh");

  TextureAsset *GetTexture(uint32_t id);

  MeshAsset *GetMesh(uint32_t id);

  void DestroyTexture(uint32_t id);

  void DestroyMesh(uint32_t id);

  void ImGui();

  vulkan::Core *Core() {
    return core_;
  }

  std::set<uint32_t> GetTextureIds() const;

  std::set<uint32_t> GetMeshIds() const;

 private:
  vulkan::Core *core_;
  uint32_t next_mesh_id_{};
  uint32_t next_texture_id_{};
  std::map<uint32_t, std::unique_ptr<TextureAsset>> textures_;
  std::map<uint32_t, std::unique_ptr<MeshAsset>> meshes_;
};
}  // namespace sparks
