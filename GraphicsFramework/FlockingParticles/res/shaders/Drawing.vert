#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// Per-object data, filled by the Transform component (Settings.json: TransformBindingPoint)
layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
} Object;

// Per-frame camera data, filled by the scene
layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

layout (location = 0) out vec3 normalVec;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 eyePos;

void main()
{
	worldPos = (Object.model * vec4(aPos, 1.0)).xyz;

	gl_Position = Camera.projection * Camera.view * Object.model * vec4(aPos, 1.0);

	normalVec = aNormal * mat3(Object.normaltr);

	eyePos = (Camera.invView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
}
