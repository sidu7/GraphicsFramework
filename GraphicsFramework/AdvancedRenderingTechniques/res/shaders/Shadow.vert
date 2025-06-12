#version 420
layout (location = 0) in vec3 aPos;

layout (binding = 0) uniform Matrices
{
	mat4 view;
	mat4 projection;
};

layout (binding = 1) uniform ObjectMatrices
{
	mat4 model;
	mat4 normaltr;
};

out vec4 position;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    position = gl_Position;
}