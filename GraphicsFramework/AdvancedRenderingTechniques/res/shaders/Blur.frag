// Start Header -------------------------------------------------------
//Copyright (C) 2018-2019 DigiPen Institute of Technology.
//Reproduction or disclosure of this file or its contents without the prior
//written consent of DigiPen Institute of Technology is prohibited.
//Author: Sidhant Tumma
//- End Header --------------------------------------------------------
#version 330 core

//Credits: LearnOpenGL Guassian blur

out vec4 FragColor;

uniform blurKernel{ float weight[101]; };

in vec2 TexCoords;

uniform sampler2D scene;
uniform int blurSize;

uniform bool horizontal;

void main()
{          


     vec2 tex_offset = 1.0 / textureSize(scene, 0); // gets size of single texel
     vec4 result = texture(scene, TexCoords) * weight[0];
     if(horizontal)
     {
         for(int i = 1; i < blurSize; ++i)
         {
            result += texture(scene, TexCoords + vec2(tex_offset.x * i, 0.0)) * weight[i];
            result += texture(scene, TexCoords - vec2(tex_offset.x * i, 0.0)) * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < blurSize; ++i)
         {
             result += texture(scene, TexCoords + vec2(0.0, tex_offset.y * i)) * weight[i];
             result += texture(scene, TexCoords - vec2(0.0, tex_offset.y * i)) * weight[i];
         }
     }
     FragColor = result;
}

