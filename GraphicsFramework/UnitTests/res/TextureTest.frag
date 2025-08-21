#version 450

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 FragCoords;

layout (location = 0) out vec4 FinalColor;

layout (binding = 0) uniform sampler2D TexSampler;

void main()
{
	FinalColor = vec4(Color, 1.0f) * texture(TexSampler, FragCoords);
}