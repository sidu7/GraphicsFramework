#version 430
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 normalVec;
out vec3 worldPos;
out vec4 worldDepth;

layout (binding = 2) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
};

layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
};

void main()
{
    TexCoords = aTexCoords;    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
	worldDepth = gl_Position;
	worldPos = (model * vec4(aPos,1.0)).xyz;

	normalVec = aNormals * mat3(normaltr);
	//vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
}

