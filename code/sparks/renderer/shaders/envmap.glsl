#ifndef ENVMAP_GLSL
#define ENVMAP_GLSL

layout(set = ENVMAP_SET, binding = 0, std140) uniform EnvmapData {
  float envmap_rotation;
  float exposure;
  int envmap_id;
  bool reflect;
}
envmap_data;

vec2 SampleEnvmapUV(vec3 direction) {
  const float pi = 3.1415926535897932384626433832795;
  direction.y = acos(direction.y) / pi;
  direction.x = atan(direction.x, direction.z) / (2.0 * pi);
  direction = direction + vec3(0.5, 0.0, 0.0);

  if (envmap_data.reflect && direction.y > 0.5) {
    direction.y = 1.0 - direction.y;
  }

  return direction.xy;
}

#endif
