#version 420
layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPosition;
layout (location = 2) out vec4 DiffuseColor;
layout (location = 3) out vec4 SpecularAlpha;

in vec3 normalVec;
in vec4 worldDepth;
in vec3 worldPos;
in vec2 TexCoords;

layout (binding = 8) uniform sampler2D texDiff;

layout (binding = 1) uniform ColorData 
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	bool lighting;
};

void main()
{    
	Normals.xyz = normalize(normalVec);
	WorldPosition.xyz = worldPos;
	WorldPosition.w = worldDepth.w;
	DiffuseColor.xyz = diffuse.xyz + texture(texDiff,TexCoords).rgb;
	SpecularAlpha.xyz = specular.xyz;
	SpecularAlpha.w = shininess;
}