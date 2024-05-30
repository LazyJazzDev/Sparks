#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 4) in vec2 in_tex_coord;
layout(location = 5) in float in_signal;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_position;
layout(location = 2) out vec4 out_normal;
layout(location = 3) out vec4 out_intensity;

#include "entity_metadata.glsl"
#include "material.glsl"

layout(set = 1, binding = 0, std140) uniform EntityMetadataUniform {
  EntityMetadata metadata;
};

layout(set = 1, binding = 1, std140) uniform EntityMaterialUniform {
  Material material;
};

layout(set = 1, binding = 2) uniform sampler2D albedo_map;
layout(set = 1, binding = 3) uniform sampler2D detail_map;

void main() {
  if (in_signal * in_pos.y < 0.0) {
    discard;
  }

  vec3 color =
      texture(albedo_map, in_tex_coord).rgb *
      texture(detail_map, in_tex_coord * material.detail_scale_offset.xy +
                              material.detail_scale_offset.zw)
          .rgb;
  vec3 normal = normalize(in_normal);
  vec3 tangent = normalize(in_tangent);
  vec3 bitangent = normalize(in_bitangent);

  vec3 light_dir = normalize(vec3(1.0, 2.0, 3.0));

  //  color *= max(dot(normal, light_dir), 0.0) * 0.7 + 0.3;
  out_albedo = vec4(1.0);
  out_position = vec4(in_pos, 0.0);
  out_normal = vec4(normal, 0.0);
  out_intensity = vec4(color * material.base_color, material.alpha);
}
