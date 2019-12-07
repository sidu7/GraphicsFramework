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
smooth in float depth;

uniform sampler2D texDiff;
uniform vec2 windowSize;
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
	
	vec2 a = (X.xy/X.w) * 0.5 + 0.5;
	vec2 b = (Xprime.xy/Xprime.w) * 0.5 + 0.5;
	//a *= windowSize;
	//b *= windowSize;
	//vec2 qx = (a - b);
	//float lenqx = length(qx);
	//if(lenqx > 0.0f)
	//{
	//	vec2 nqx = normalize(qx);
	//	VelocityDepth.xy = nqx * clamp(lenqx,0.5f,float(k));
	//}
	//else
	//{
	//	VelocityDepth.xy = vec2(0.0f);
	//}
	VelocityDepth.xy = a - b;

	VelocityDepth.z = depth;
}