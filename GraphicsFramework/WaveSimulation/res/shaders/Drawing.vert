#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Per-object matrices, filled by the scene (Settings.json: TransformBindingPoint)
layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
} Object;

// Per-frame camera, filled by the scene
layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

layout (location = 0) out vec3 worldPos;
layout (location = 1) out vec3 worldNormal;

void main()
{
	vec4 worldPosition = Object.model * vec4(aPos, 1.0);

	gl_Position = Camera.projection * Camera.view * worldPosition;

	worldPos = worldPosition.xyz;
	worldNormal = normalize((Object.normaltr * vec4(aNormal, 0.0)).xyz);
}
