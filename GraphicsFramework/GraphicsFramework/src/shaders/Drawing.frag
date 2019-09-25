#version 330 core
layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPosition;
layout (location = 2) out vec4 DiffuseColor;
layout (location = 3) out vec4 SpecularAlpha;

in vec3 normalVec;
in vec3 worldPos;

uniform sampler2D texture_diffuse1;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

void main()
{    
	Normals.xyz = normalVec;
	WorldPosition.xyz = worldPos;
	DiffuseColor.xyz = diffuse;
	SpecularAlpha.xyz = specular;
	SpecularAlpha.w = shininess;
}