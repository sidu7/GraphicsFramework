/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Rendering/Renderer.h"
#include <string>

class Texture
{
public:
	virtual ~Texture() {}

	virtual void Init(int channels, int width, int height) = 0;
	virtual void Init(const std::string& FilePath) = 0;
	virtual void Init(void* buffer, int size) = 0;

	virtual void EnableTiling() const = 0;
	virtual inline std::string GetSourcePath() const = 0;
};