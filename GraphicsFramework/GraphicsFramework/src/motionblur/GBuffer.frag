#version 330 core
layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPosition;
layout (location = 2) out vec4 DiffuseColor;
layout (location = 3) out vec4 SpecularAlpha;
layout (location = 4) out vec4 VelocityDepth;

in vec3 normalVec;
in vec3 worldPos;
in vec2 TexCoords;
smooth in vec4 X;
smooth in vec4 Xprime;
in float depth;

uniform sampler2D texDiff;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform float deltaTime;
uniform int k;

void main()
{    
	Normals.xyz = normalize(normalVec);
	WorldPosition.xyz = worldPos;
	DiffuseColor.xyz = diffuse + texture(texDiff,TexCoords).rgb;
	SpecularAlpha.xyz = specular;
	SpecularAlpha.w = shininess;
	
	//vec2 qx = 0.5*(X - Xprime) * deltaTime;
	//float lenqx = length(qx);
	//VelocityDepth.xy = qx*max(0.5,min(lenqx,k))/(lenqx + 0.01f);
	vec2 a = (X.xy/X.w) * 0.5 + 0.5;
	vec2 b = (Xprime.xy/Xprime.w) * 0.5 + 0.5;
	VelocityDepth.xy = a - b;

	VelocityDepth.z = depth;
}