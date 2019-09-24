#version 330

layout (location = 0) out vec4 Normals;
layout (location = 1) out vec4 WorldPos;
layout (location = 2) out vec4 DiffuseKd;
layout (location = 3) out vec4 SpecularAlpha;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;
in vec4 shadowCoord;

uniform int objectId;
uniform vec3 diffuse; //Kd
uniform vec3 specular; //Ks
uniform float shininess; //alpha exponent

uniform vec3 Light; //Ii
uniform vec3 Ambient; //Ia
uniform bool UseBRDF; //Lighting flag

uniform sampler2D shadowMap;

const float pi = 22/7;

vec2 shadowIndex;

void main()
{
   Normals = vec4(normalVec,0.0);
}
