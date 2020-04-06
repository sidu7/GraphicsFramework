#version 330

uniform vec3 light_position;
uniform vec3 diffuse_color;

in vec3 world_position;
in vec3 world_normal;

out vec4 frag_color;

void main(void) {

vec3 l = normalize(light_position - world_position),
    m = normalize(world_normal);
float ml = max(0.0,dot(m,l));
vec3 color = ml * diffuse_color;
frag_color = vec4(color,1);
}