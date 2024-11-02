#pragma once
#include "glm/gtc/matrix_transform.hpp"
#include "sparks/asset_manager/asset_manager.h"
#include "sparks/scene/scene.h"

namespace sparks {

std::vector<std::pair<std::string, std::function<void(Scene *scene)>>>
BuiltInSceneList();

void LoadCornellBox(Scene *scene);

void LoadIslandScene(Scene *scene);

}  // namespace sparks
