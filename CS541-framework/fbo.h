#pragma once
#include "glbinding/gl/gl.h"
#include <iostream>

using namespace gl;

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x,__FILE__,__LINE__))

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

class FrameBuffer
{
private:
	unsigned int m_RendererID;

public:
	unsigned int* m_TextureID;
	unsigned int mWidth, mHeight, mDepthBuffer;
	unsigned int mTexCount;
public:
	FrameBuffer(int width, int height, int Texcount = 1);
	FrameBuffer() { }
	~FrameBuffer();

	void Bind() const;
	void Unbind() const;
	void TexBind(unsigned int index = 0, unsigned int slot = 0);
	void TexUnbind(unsigned int index = 0);
	void Clear() const;
	void DepthBind();
	void DepthUnbind();
	void Delete() const;
	void ChangeSize(unsigned int width, unsigned int height);
};