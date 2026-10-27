#version 450

layout (location = 0) in vec3 normalVec;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 eyePos;

layout (location = 0) out vec4 FragColor;

// Per-material data, filled by the Material component (Settings.json: MaterialBindingPoint)
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

const float pi = 22.0 / 7.0;

void main()
{
	if (Material.lighting != 0)
	{
		vec3 lightVec = Light.position.xyz - worldPos;
		vec3 eyeVec = eyePos - worldPos;

		vec3 N = normalize(normalVec);
		vec3 L = normalize(lightVec);
		vec3 V = normalize(eyeVec);
		vec3 H = normalize(L + V);
		float NL = max(dot(L, N), 0.0);
		float NH = max(dot(N, H), 0.0);
		float LH = max(dot(L, H), 0.0);

		vec3 Kd = Material.diffuse.rgb;
		vec3 Ks = Material.specular.rgb;

		float alp = Material.shininess;
		vec3 Ii = Light.color.rgb;

		vec3 F = Ks + (vec3(1.0) - Ks) * pow(1.0 - LH, 5.0);
		float D = (alp + 2.0) * pow(NH, alp) / (2.0 * pi);

		vec3 BRDF = (Kd / pi) + ((F * D) / (4.0 * pow(LH, 2.0)));

		FragColor = vec4(Ii * NL * BRDF, 1.0);
	}
	else
	{
		FragColor = vec4(Material.diffuse.rgb, 1.0);
	}
}
