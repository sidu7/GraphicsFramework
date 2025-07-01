#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoord;

layout (binding = 2) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 inverseview;
};

layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
};

void main()
{
	 gl_Position = projection * view * model * vec4(aPos, 1.0);
	 TexCoord = gl_Position.xy;
}