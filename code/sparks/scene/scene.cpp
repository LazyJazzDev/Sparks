#include "sparks/scene/scene.h"

namespace sparks {
Scene::Scene(struct Renderer *renderer,
             struct AssetManager *asset_manager,
             int max_entities)
    : renderer_(renderer), asset_manager_(asset_manager) {
}
}  // namespace sparks
