#version 450

layout (binding = 2) uniform Matrices
{
	mat4 Perspective;
	mat4 View;
	mat4 Model;
} MatrixData;

layout (location = 0) in vec2 Position;
layout (location = 1) in vec3 Color;

layout (location = 0) out vec3 OutColor;

void main()
{
	gl_Position = MatrixData.Perspective * MatrixData.View * MatrixData.Model * vec4(Position, 0.0f, 1.0f);
	OutColor = Color;
}