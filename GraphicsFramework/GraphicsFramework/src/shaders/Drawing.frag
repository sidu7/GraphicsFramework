#version 330 core
layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPosition;
layout (location = 2) out vec4 DiffuseColor;
layout (location = 3) out vec4 SpecularAlpha;

in vec3 normalVec;
in vec3 worldPos;
in vec2 TexCoords;

uniform sampler2D texDiff;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

void main()
{    
	Normals.xyz = normalize(normalVec);
	WorldPosition.xyz = worldPos;
	DiffuseColor.xyz = diffuse + texture(texDiff,TexCoords).rgb;
	SpecularAlpha.xyz = specular;
	SpecularAlpha.w = shininess;
}