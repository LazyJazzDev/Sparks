#ifndef SCENE_SETTINGS_H
#define SCENE_SETTINGS_H

struct SceneSettings {
  mat4 projection;
  mat4 inv_projection;
  mat4 world_to_camera;
  mat4 camera_to_world;
};

#endif
