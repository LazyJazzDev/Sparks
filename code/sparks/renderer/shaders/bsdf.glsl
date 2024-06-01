#ifndef BSDF_GLSL
#define BSDF_GLSL
#include "constants.glsl"
#include "hit_record.glsl"
#include "material.glsl"
#include "random.glsl"

#define Ng hit_record.geometry_normal
#define N material.normal
#define V hit_record.omega_v
#define I hit_record.omega_v
#define omega_in L

vec3 EvalLambertianBSDF(in vec3 L, out float pdf) {
  float cos_pi = max(dot(N, omega_in), 0.0f) * INV_PI;
  pdf = cos_pi;
  return vec3(cos_pi) * material.base_color;
}

void SampleLambertianBSDF(out vec3 eval, out vec3 L, out float pdf) {
  SampleCosHemisphere(N, L, pdf);
  if (dot(Ng, L) > 0.0) {
    eval = vec3(pdf) * material.base_color;
  } else {
    pdf = 0.0f;
    eval = vec3(0.0);
  }
}

vec3 EvalBSDF(in vec3 L, out float pdf) {
  return EvalLambertianBSDF(L, pdf);
}

void SampleBSDF(out vec3 eval, out vec3 L, out float pdf) {
  SampleLambertianBSDF(eval, L, pdf);
}

#undef Ng
#undef N
#undef V
#undef I
#undef omega_in

#endif
