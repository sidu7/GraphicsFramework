#version 450
layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPosition;
layout (location = 2) out vec4 DiffuseColor;
layout (location = 3) out vec4 SpecularAlpha;
layout (location = 4) out vec4 VelocityDepth;

layout(location = 1) in vec3 normalVec;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec2 TexCoords;
layout(location = 4) in vec4 X;
layout(location = 5) in vec4 Xprime;
layout(location = 6) in float depth;

layout (binding = 8) uniform sampler2D texDiff;

layout (binding = 3) uniform ColorData 
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	bool lighting;
} colorData;

layout (binding = 9) uniform GBufferParams
{
	vec2 windowSize;
	float deltaTime;
	int k;
} gbufferUbo;

void main()
{    
	Normals.xyz = normalize(normalVec);
	WorldPosition.xyz = worldPos;
	DiffuseColor.xyz = colorData.diffuse.xyz + texture(texDiff,TexCoords).rgb;
	SpecularAlpha.xyz = colorData.specular.xyz;
	SpecularAlpha.w = colorData.shininess;
	
	vec2 a = (X.xy/X.w) * 0.5 + 0.5;
	vec2 b = (Xprime.xy/Xprime.w) * 0.5 + 0.5;
	VelocityDepth.xy = a - b;

	VelocityDepth.z = depth;
}
