#version 420
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 normalVec;
out vec3 worldPos;
smooth out vec4 X;
smooth out vec4 Xprime;
smooth out float depth;

layout (binding = 0) uniform Matrices
{
	mat4 view;
	mat4 projection;
	mat4 prevview;
};

layout (binding = 1) uniform ObjectMatrices
{
	mat4 model;
	mat4 normaltr;
	mat4 prevmodel;
};

void main()
{
    TexCoords = aTexCoords;    
    vec4 gpos =  view * model * vec4(aPos, 1.0);
	depth = gpos.z;
	X = projection * gpos;
	Xprime = (projection * prevview * prevmodel * vec4(aPos,1.0));

	worldPos = (model * vec4(aPos,1.0)).xyz;
	gl_Position = X;
	normalVec = aNormals * mat3(normaltr);
	//vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
}

