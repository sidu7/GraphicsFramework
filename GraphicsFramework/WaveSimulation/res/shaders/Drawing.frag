#version 450

layout (location = 0) in vec3 worldPos;
layout (location = 1) in vec3 worldNormal;

layout (location = 0) out vec4 FragColor;

// Per-material data (Settings.json: MaterialBindingPoint)
layout (binding = 1) uniform MaterialData
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	int lighting;
} Material;

// Per-frame light data, filled by the scene
layout (binding = 3) uniform LightData
{
	vec4 position;
	vec4 color;
} Light;

void main()
{
	vec3 L = normalize(Light.position.xyz - worldPos);
	vec3 N = normalize(worldNormal);

	float NL = max(dot(N, L), 0.0);

	FragColor = vec4(NL * Material.diffuse.rgb, 1.0);
}
