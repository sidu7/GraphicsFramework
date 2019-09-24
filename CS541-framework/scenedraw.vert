#version 330

uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr, NormalTr, ShadowMat;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

out vec3 normalVec, lightVec, eyeVec;
out vec3 worldPos;
out vec2 texCoord;
out vec4 shadowCoord;
uniform vec3 lightPos;

void main()
{      
    gl_Position = WorldProj * WorldView * ModelTr * vertex;
    
    shadowCoord = ShadowMat * ModelTr * vertex;

    worldPos = (ModelTr*vertex).xyz;

    vec3 eyePos = (WorldInverse * vec4(0,0,0,1)).xyz;

    normalVec = vertexNormal*mat3(NormalTr); 
    lightVec = lightPos - worldPos;
    eyeVec = eyePos - worldPos;

    texCoord = vertexTexture; 
}
