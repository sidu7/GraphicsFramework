#version 330 core

out vec4 FragColor;

const float pi = 22/7;

in vec2 TexCoord;

uniform vec3 Ambient;
uniform sampler2D diffusetex;
uniform sampler2D normaltex;
uniform sampler2D irradiance;
uniform float exposure;
uniform float contrast;

void main()
{
	vec3 Kd = texture(diffusetex,TexCoord).xyz;
	vec3 N = normalize(texture(normaltex,TexCoord).xyz);
	vec2 uv = vec2(1/2 - atan(N.z,N.x)/(2*pi),  acos(-N.y)/pi);
	vec3 IrrAmbient = texture(irradiance,uv).xyz;
	vec4 OutColor = vec4(IrrAmbient * Kd / pi,1.0);

	// Tone Mapping and Gamma Correction
	vec4 base = exposure * OutColor / (exposure * OutColor + vec4(1,1,1,1));

	FragColor = pow(base,vec4(contrast/2.2));
}