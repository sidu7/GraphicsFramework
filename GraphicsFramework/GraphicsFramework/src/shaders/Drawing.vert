#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 normalVec;
out vec3 worldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normaltr;

void main()
{
    TexCoords = aTexCoords;    
    gl_Position = projection * view * model * vec4(aPos, 1.0);

	worldPos = (model * vec4(aPos,1.0)).xyz;

	normalVec = aPos * mat3(normaltr);
	//vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
}

