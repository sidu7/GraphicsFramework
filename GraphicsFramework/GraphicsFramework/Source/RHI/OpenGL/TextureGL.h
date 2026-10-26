/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "GLEW/include/GL/glew.h"

#include "Rendering/Texture.h"

namespace rhi::gl
{
class TextureGL : public Texture
{
public:
	TextureGL();
	virtual ~TextureGL();

	virtual void Init(ImageFormat format, int width, int height) override;
	virtual void Init(const std::string& FilePath) override;
	virtual void Init(void* buffer, int size) override;

	virtual void EnableTiling() const override;
	virtual inline std::string GetSourcePath() const override { return mFilePath; }

	GLenum GetGlFormat();
	GLenum GetGlInternalFormat();

	int mWidth, mHeight;
	std::string mFilePath;
	unsigned int mRendererID;

protected:
	void ReadBufferToTexture();

	uint8_t* mLocalBuffer;
};
}
