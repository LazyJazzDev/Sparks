#ifndef ENVMAP_GLSL
#define ENVMAP_GLSL

#include "constants.glsl"

layout(set = ENVMAP_SET, binding = 0, std140) uniform EnvmapData {
  float envmap_rotation;
  float exposure;
  int envmap_id;
  bool reflect;
}
envmap_data;

vec2 SampleEnvmapUV(vec3 direction) {
  direction.y = acos(direction.y) / PI;
  direction.x = atan(-direction.x, direction.z) / (2.0 * PI);

  if (envmap_data.reflect && direction.y > 0.5) {
    direction.y = 1.0 - direction.y;
  }

  return direction.xy;
}

#endif
