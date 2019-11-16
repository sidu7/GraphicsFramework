#version 330 core

out vec4 FragColor;

const float pi = 22/7;

in vec3 worldPos;

uniform sampler2D skyDome;
uniform mat4 inverseview;
uniform float exposure;
uniform float contrast;

void main()
{
	vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
	vec3 eyeVec = eyePos - worldPos;
	vec3 V = normalize(eyeVec);

	vec2 uv = vec2(1/2 - atan(V.z,V.x)/(2*pi),  acos(V.y)/pi);

	vec4 OutColor = vec4(texture(skyDome,uv).rgb,1.0);

	// Tone Mapping and Gamma Correction
	vec4 base = exposure * OutColor / (exposure * OutColor + vec4(1,1,1,1));

	FragColor = pow(base,vec4(contrast/2.2));
}