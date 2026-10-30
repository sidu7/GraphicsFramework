#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
} Object;

layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

layout (location = 0) out vec3 normalVec;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec2 texCoord;

void main()
{
	worldPos = (Object.model * vec4(aPos, 1.0)).xyz;
	gl_Position = Camera.projection * Camera.view * vec4(worldPos, 1.0);
	normalVec = aNormal * mat3(Object.normaltr);
	texCoord = aTexCoord;
}
