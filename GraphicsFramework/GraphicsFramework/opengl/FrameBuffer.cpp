/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/
#include "FrameBuffer.h"
#include <iostream>
#include "Renderer.h"

FrameBuffer::FrameBuffer(int width, int height, int Texcount) : mWidth(width), mHeight(height), mTexCount(Texcount), m_TextureID(nullptr)
{
	GLCall(glGenFramebuffers(1, &m_RendererID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));

	m_TextureID = new unsigned int[mTexCount];
	unsigned int* attachments = new unsigned int[mTexCount];
	
	// create floating point color buffers
	glGenTextures(mTexCount, m_TextureID);
	for (unsigned int i = 0; i < mTexCount; i++)
	{
		GLCall(glBindTexture(GL_TEXTURE_2D, m_TextureID[i]));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, NULL));
		
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		// attach texture to framebuffer
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_TextureID[i], 0));
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	// create and attach depth buffer (renderbuffer)
	unsigned int rboDepth;
	GLCall(glGenRenderbuffers(1, &mDepthBuffer));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer));

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	GLCall(glDrawBuffers(mTexCount, attachments));

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Error building FrameBuffer" << std::endl;

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	delete[] attachments;
}

FrameBuffer::~FrameBuffer()
{
	delete[] m_TextureID;
	Delete();
}

void FrameBuffer::Bind() const
{
	glViewport(0, 0,mWidth, mHeight);
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));
	Clear();
}

void FrameBuffer::Unbind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	Clear();
}

void FrameBuffer::TexBind(unsigned int index, unsigned int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_TextureID[index]);
}

void FrameBuffer::TexUnbind(unsigned int index)
{

}

void FrameBuffer::Clear() const
{
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::DepthBind()
{

}

void FrameBuffer::DepthUnbind()
{

}

void FrameBuffer::Delete() const
{
	GLCall(glDeleteFramebuffers(1, &m_RendererID));
}

void FrameBuffer::ChangeSize(unsigned int width, unsigned int height)
{
	mHeight = height;
	mWidth = width;
	for (unsigned int i = 0; i < mTexCount; i++)
	{
		GLCall(glBindTexture(GL_TEXTURE_2D, m_TextureID[i]));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, 0));

		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer));
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight));
	}
}
