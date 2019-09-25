#version 330

layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPos;
layout (location = 2) out vec4 DiffuseKd;
layout (location = 3) out vec4 SpecularAlpha;

in vec3 normalVec;
in vec2 texCoord;
in vec3 worldPos;

uniform vec3 diffuse; //Kd
uniform vec3 specular; //Ks
uniform float shininess; //alpha exponent

const float pi = 22/7;

void main()
{
   Normals = vec4(normalVec,0.0);
   DiffuseKd = vec4(diffuse,1.0);
   SpecularAlpha = vec4(specular,shininess);
   WorldPos = vec4(worldPos,1.0);
}
