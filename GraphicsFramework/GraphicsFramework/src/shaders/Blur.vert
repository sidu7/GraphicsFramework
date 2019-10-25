// Start Header -------------------------------------------------------
//Copyright (C) 2018-2019 DigiPen Institute of Technology.
//Reproduction or disclosure of this file or its contents without the prior
//written consent of DigiPen Institute of Technology is prohibited.
//Author: Sidhant Tumma
//- End Header --------------------------------------------------------
#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 TexCoord;

out vec2 TexCoords;

void main()
{
    TexCoords = TexCoord;
    gl_Position = vec4(position,0.0, 1.0);
}