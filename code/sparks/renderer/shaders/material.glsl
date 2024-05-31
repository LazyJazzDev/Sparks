#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material {
  vec3 base_color;
  float alpha;
  vec3 normal;
  float padding0;
};

#endif
