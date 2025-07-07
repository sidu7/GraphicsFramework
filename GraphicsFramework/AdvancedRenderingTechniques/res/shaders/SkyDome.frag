#version 430 core

out vec4 FragColor;

const float pi = 22.0f/7.0f;

in vec3 fragPos;

layout (binding = 1) uniform sampler2D skyDome;

layout (binding = 4) uniform LightData
{
	vec3 Ambient;
	vec3 Light;
	float Exposure;
	float Contrast;
	bool ShowIBLDiffuse;
	bool ShowIBLSpecular;
	int ShowGBuffer;
};

void main()
{
	vec3 V = normalize(fragPos);

	vec2 uv = vec2(1/2 - atan(V.z,V.x)/(2*pi),  acos(-V.y)/pi);

	vec4 OutColor = vec4(texture(skyDome,uv).rgb,1.0);

	// Tone Mapping and Gamma Correction
	vec4 base = Exposure * OutColor / (Exposure * OutColor + vec4(1,1,1,1));

	FragColor = pow(base,vec4(Contrast/2.2));
}