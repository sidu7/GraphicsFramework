// Start Header -------------------------------------------------------
//Copyright (C) 2018-2019 DigiPen Institute of Technology.
//Reproduction or disclosure of this file or its contents without the prior
//written consent of DigiPen Institute of Technology is prohibited.
//Author: Sidhant Tumma
//- End Header --------------------------------------------------------
#version 450

//Credits: LearnOpenGL Guassian blur

layout(location = 0) out vec4 FragColor;

layout (binding = 0) uniform blurKernel{ float weight[101]; };

layout(location = 0) in vec2 TexCoords;

layout (binding = 1) uniform sampler2D scene;
layout (binding = 2) uniform BlurParams { int blurSize; bool horizontal; } blurParams;

void main()
{   
     vec2 tex_offset = 1.0 / textureSize(scene, 0); // gets size of single texel
     vec4 result = texture(scene, TexCoords);// * weight[blurParams.blurSize];
     if(blurParams.horizontal)
     {
         for(int i = blurParams.blurSize - 1; i >= 0; --i)
         {
            //result += texture(scene, TexCoords + vec2(tex_offset.x * i, 0.0)) * weight[i];
           //result += texture(scene, TexCoords - vec2(tex_offset.x * i, 0.0)) * weight[i];
         }
     }
     else
     {
         for(int i = blurParams.blurSize - 1; i >= 0; --i)
         {
             //result += texture(scene, TexCoords + vec2(0.0, tex_offset.y * i)) * weight[i];
             //result += texture(scene, TexCoords - vec2(0.0, tex_offset.y * i)) * weight[i];
         }
     }
     FragColor = result;
}

