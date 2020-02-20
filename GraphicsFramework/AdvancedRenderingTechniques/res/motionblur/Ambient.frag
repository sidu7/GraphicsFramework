#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 Ambient;
uniform sampler2D diffusetex;

void main()
{
	FragColor = vec4(Ambient * texture(diffusetex,TexCoord).xyz,1.0);
}