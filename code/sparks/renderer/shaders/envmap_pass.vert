#version 450
#define SCENE_SETTINGS_SET 0
#include "scene_settings.glsl"

const vec2[6] positions = vec2[6](vec2(-1.0, -1.0),
                                  vec2(1.0, -1.0),
                                  vec2(-1.0, 1.0),
                                  vec2(-1.0, 1.0),
                                  vec2(1.0, -1.0),
                                  vec2(1.0, 1.0));

layout(location = 0) out vec3 ray_direction;

void main() {
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  mat4 proj = scene_settings.projection;
  proj[0][0] = 1.0 / proj[0][0];
  proj[1][1] = 1.0 / proj[1][1];
  ray_direction =
      normalize((gl_Position * vec4(1.0, -1.0, 0.0, 1.0)) * proj).xyz *
      mat3(scene_settings.world_to_camera);
}
