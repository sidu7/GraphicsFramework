#version 330

uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr, NormalTr;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

out vec3 normalVec;
out vec3 worldPos;
out vec2 texCoord;

void main()
{      
    gl_Position = WorldProj * WorldView * ModelTr * vertex;
        
    worldPos = (ModelTr*vertex).xyz;
  
    normalVec = vertexNormal*mat3(NormalTr); 
   
    texCoord = vertexTexture; 
}
