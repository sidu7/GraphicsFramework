/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "FrameBuffer_OpenGL.h"

#include "Core/Core.h"
#include "Rendering/OpenGL/Renderer_OpenGL.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/OpenGL/Texture_OpenGL.h"

FrameBuffer_OpenGL::FrameBuffer_OpenGL() :
	m_RendererID(0),
	mWidth(0), 
	mHeight(0), 
	mDepthBuffer(0),
	mTexCount(0)
{

}

FrameBuffer_OpenGL::~FrameBuffer_OpenGL()
{
	std::for_each(m_Textures.begin(), m_Textures.end(),
				  [](Texture* x)
	{
		delete x;
	});
	Delete();
}

void FrameBuffer_OpenGL::Init(int width, int height, ImageFormat format, int Texcount)
{
	mWidth = width;
	mHeight = height;
	mTexCount = Texcount;

	GLCall(glGenFramebuffers(1, &m_RendererID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID));

	unsigned int* attachments = new unsigned int[mTexCount];
	
	// create floating point color buffers
	for (unsigned int i = 0; i < mTexCount; i++)
	{
		Texture_OpenGL* NewTexture = static_cast<Texture_OpenGL*>(RenderingFactory::Instance()->CreateTexture());

		NewTexture->Init(format, mWidth, mHeight);
		m_Textures.push_back(NewTexture);

		// attach texture to framebuffer
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, NewTexture->mRendererID, 0));
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

void FrameBuffer_OpenGL::CopyDepthTo(const FrameBuffer* target)
{
	const FrameBuffer_OpenGL* targetOpengl = static_cast<const FrameBuffer_OpenGL*>(target);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_RendererID);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetOpengl->m_RendererID);
	glBlitFramebuffer(0, 0, mWidth, mHeight,
					  0, 0, targetOpengl->mWidth, targetOpengl->mHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void FrameBuffer_OpenGL::Clear() const
{
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer_OpenGL::DepthBind()
{

}

void FrameBuffer_OpenGL::DepthUnbind()
{

}

void FrameBuffer_OpenGL::Delete() const
{
	GLCall(glDeleteFramebuffers(1, &m_RendererID));
}

void FrameBuffer_OpenGL::ChangeSize(unsigned int width, unsigned int height)
{
	mHeight = height;
	mWidth = width;

	for (unsigned int i = 0; i < mTexCount; i++)
	{
		Renderer::Instance()->BindTexture(m_Textures[i]);
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, 0));

		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer));
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight));
	}
}

const Texture* FrameBuffer_OpenGL::GetTexture(unsigned int index)
{
	if (index < m_Textures.size())
	{
		return m_Textures[index];
	}
	return nullptr;
}
