#version 450

layout(set = 0, binding = 0, std140) uniform SceneSettings {
  mat4 projection;
  mat4 inv_projection;
  mat4 world_to_camera;
  mat4 camera_to_world;
}
scene_settings;

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
