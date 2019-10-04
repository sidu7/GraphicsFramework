#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normaltr;
uniform mat4 inverseview;

out vec3 normalVec;
out vec3 worldPos;
out vec3 eyePos;

void main()
{
	worldPos = (model * vec4(aPos,1.0)).xyz;
	gl_Position = projection * view * model * vec4(aPos,1.0);

	normalVec = aNormal * mat3(normaltr);

	eyePos = (inverseview * vec4(0,0,0,1)).xyz;
}