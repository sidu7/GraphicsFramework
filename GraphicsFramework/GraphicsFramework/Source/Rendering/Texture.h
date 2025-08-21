/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Rendering/Renderer.h"
#include <string>

typedef uint16_t half_float;

enum class ImageFormat
{
	Invalid,
	R8,
	RG8,
	RGB8,
	RGBA8,
	R16F,
	RG16F,
	RGB16F,
	RGBA16F,
	R32F,
	RG32F,
	RGB32F,
	RGBA32F,
};

class Texture
{
public:
	Texture() : mFormat(ImageFormat::Invalid) {}
	virtual ~Texture() {}

	virtual void Init(ImageFormat format, int width, int height) = 0;
	virtual void Init(const std::string& FilePath) = 0;
	virtual void Init(void* buffer, int size) = 0;

	virtual void EnableTiling() const = 0;
	virtual inline std::string GetSourcePath() const = 0;

	template<typename ColorBitsType>
	ImageFormat GetImageFormat(int NumChannels)
	{
		std::cout << "Invalid ImageFormat: Unsupported TexelType specified." << std::endl;
		return ImageFormat::Invalid;
	}

	template<>
	ImageFormat GetImageFormat<uint8_t>(int NumChannels)
	{
		switch (NumChannels) {

			case 1:
				return ImageFormat::R8;
			case 2:
				return ImageFormat::RG8;
			case 3:
				return ImageFormat::RGB8;
			case 4:
				return ImageFormat::RGBA8;
		}
	}

	template<>
	ImageFormat GetImageFormat<half_float>(int NumChannels)
	{
		switch (NumChannels) {

			case 1:
				return ImageFormat::R16F;
			case 2:
				return ImageFormat::RG16F;
			case 3:
				return ImageFormat::RGB16F;
			case 4:
				return ImageFormat::RGBA16F;
		}
	}

	template<>
	ImageFormat GetImageFormat<float>(int NumChannels)
	{
		switch (NumChannels) {

			case 1: 
				return ImageFormat::R32F;
			case 2:
				return ImageFormat::RG32F;
			case 3:
				return ImageFormat::RGB32F;
			case 4: 
				return ImageFormat::RGBA32F;
		}
	}

	int GetNumChannels() const
	{
		switch (mFormat) {
			case ImageFormat::R8:
			case ImageFormat::R16F:
			case ImageFormat::R32F:
				return 1;
			case ImageFormat::RG8:
			case ImageFormat::RG16F:
			case ImageFormat::RG32F:
				return 2;
			case ImageFormat::RGB8:
			case ImageFormat::RGB16F:
			case ImageFormat::RGB32F:
				return 3;
			case ImageFormat::RGBA8:
			case ImageFormat::RGBA16F:
			case ImageFormat::RGBA32F:
				return 4;
		}
	}

	ImageFormat mFormat;
};