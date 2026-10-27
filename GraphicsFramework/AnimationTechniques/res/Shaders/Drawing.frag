#version 450

layout (location = 0) in vec3 normalVec;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform MaterialData
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	int lighting;
	int useTexture;
	int pad0;
	int pad1;
	int pad2;
} Material;

layout (binding = 3) uniform LightData
{
	vec4 position;
	vec4 color;
} Light;

layout (binding = 8) uniform sampler2D meshTexture;

void main()
{
	vec3 albedo = Material.diffuse.rgb;

	if (Material.useTexture != 0)
	{
		albedo *= texture(meshTexture, texCoord).rgb;
	}

	if (Material.lighting == 0)
	{
		FragColor = vec4(albedo, 1.0);
		return;
	}

	vec3 N = normalize(normalVec);
	vec3 L = normalize(Light.position.xyz - worldPos);
	float NL = max(dot(N, L), 0.0);

	FragColor = vec4(Light.color.rgb * NL * albedo, 1.0);
}
