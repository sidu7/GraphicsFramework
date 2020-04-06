#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 model_trans;
uniform mat4 normal_trans;
uniform mat4 vp;

out vec3 world_position;
out vec3 world_normal;

void main() {
  vec4 P = model_trans * vec4(position,1);
  vec4 m = normal_trans * vec4(normal,0);
  gl_Position = vp * P;
  world_position = P.xyz;
  world_normal = normalize(m.xyz);
}