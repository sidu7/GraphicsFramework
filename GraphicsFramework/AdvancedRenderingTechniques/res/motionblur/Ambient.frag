#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoord;

layout(binding = 0) uniform AmbientUBO
{
    vec3 Ambient;
} ambientUbo;

layout(binding = 1) uniform sampler2D diffusetex;

void main()
{
    FragColor = vec4(ambientUbo.Ambient * texture(diffusetex,TexCoord).xyz,1.0);
}
