#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

const int MAX_BONES = 100;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normaltr;
uniform mat4 inverseview;
uniform mat4 boneMatrix[MAX_BONES];

out vec3 normalVec;
out vec3 worldPos;
out vec3 eyePos;

void main()
{

	mat4 BoneTransform = boneMatrix[aBoneIDs[0]] * aWeights[0];
	BoneTransform += boneMatrix[aBoneIDs[1]] * aWeights[1];
	BoneTransform += boneMatrix[aBoneIDs[2]] * aWeights[2];
	BoneTransform += boneMatrix[aBoneIDs[3]] * aWeights[3];

	gl_Position = projection * view * model * BoneTransform * vec4(aPos,1.0);

	worldPos = (model * BoneTransform * vec4(aPos,1.0)).xyz;

	vec4 normalL = vec4(aNormal,0.0) * BoneTransform;

	normalVec = (normalL * normaltr).xyz;

	eyePos = (inverseview * vec4(0,0,0,1)).xyz;
}