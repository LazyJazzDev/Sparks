#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material {
  vec3 base_color;
  float alpha;
  vec4 detail_scale_offset;
};

#endif
