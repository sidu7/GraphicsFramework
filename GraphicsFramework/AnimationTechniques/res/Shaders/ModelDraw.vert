#version 450

// Mesh::SetupMesh layout: Vec3 position, Vec3 normal, Vec2 uv, iVec4 boneIDs, Vec4 weights
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

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

layout (binding = 4) uniform BoneMatrices
{
	mat4 bones[128];
} Skeleton;

layout (location = 0) out vec3 normalVec;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec2 texCoord;

void main()
{
	mat4 skin =
		Skeleton.bones[aBoneIDs.x] * aWeights.x +
		Skeleton.bones[aBoneIDs.y] * aWeights.y +
		Skeleton.bones[aBoneIDs.z] * aWeights.z +
		Skeleton.bones[aBoneIDs.w] * aWeights.w;

	vec4 skinnedPosition = skin * vec4(aPos, 1.0);
	vec3 skinnedNormal = mat3(skin) * aNormal;

	worldPos = (Object.model * skinnedPosition).xyz;
	gl_Position = Camera.projection * Camera.view * vec4(worldPos, 1.0);
	normalVec = skinnedNormal * mat3(Object.normaltr);
	texCoord = aTexCoord;
}
