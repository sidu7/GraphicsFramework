#version 430 core

out vec4 FragColor;

const float pi = 22.0f/7.0f;

in vec3 fragPos;

layout (binding = 1) uniform sampler2D skyDome;
uniform float exposure;
uniform float contrast;

void main()
{
	vec3 V = normalize(fragPos);

	vec2 uv = vec2(1/2 - atan(V.z,V.x)/(2*pi),  acos(-V.y)/pi);

	vec4 OutColor = vec4(texture(skyDome,uv).rgb,1.0);

	// Tone Mapping and Gamma Correction
	vec4 base = exposure * OutColor / (exposure * OutColor + vec4(1,1,1,1));

	FragColor = pow(base,vec4(contrast/2.2));
}