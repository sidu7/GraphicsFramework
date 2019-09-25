/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Renderer.h"
#include "VertexArray.h"
#include "ElementArrayBuffer.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include <iostream>
#include "Shader.h"

extern Shader *gpShader;
extern Shader *gdShader;

void GLClearError()
{
	//while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGl Error] (" << std::hex << error << std::dec << "):" << function << " " << file << ":" << line << std::endl;
		return false;
	}
	return true;
}

void Renderer::Init(SDL_Window * window)
{
	pWindow = window;

	//Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		printf("Error initializing GLEW\n");
	}

	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glBlendFunc(GL_ONE, GL_ONE));

	InitPrimitiveModels();
}

void Renderer::Clear() const
{
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void Renderer::SwapBuffers() const
{
	SDL_GL_SwapWindow(pWindow);
}

void Renderer::Draw(const VertexArray& va, const ElementArrayBuffer& ib, const Shader& shader) const
{
	shader.Bind();
	va.Bind();
	ib.Bind();
	GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::DebugDraw(const VertexArray& va, const ElementArrayBuffer& ib, const Shader& shader) const
{
	shader.Bind();
	va.Bind();
	ib.Bind();
	GLCall(glDrawElements(GL_LINE_LOOP, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::DrawDebugCircle(const VertexArray& va, const Shader& shader) const
{
	shader.Bind();
	va.Bind();
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 16));
}

void Renderer::DrawDebugLine(const VertexArray& va, const Shader& shader) const
{
	shader.Bind();
	va.Bind();
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 2));
}

void Renderer::DrawQuad()
{
	//Quad
	float positions_quad[] = {
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
	};

	unsigned int indices_quad[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexArray VAO_quad;
	VertexBuffer VBO_quad;
	ElementArrayBuffer IBO_quad;

	VBO_quad.AddData(positions_quad, 5 * 4 * sizeof(float));
	IBO_quad.AddData(indices_quad, 6, sizeof(unsigned int));

	VAO_quad.AddBuffer(VBO_quad);
	VAO_quad.Push(3, GL_FLOAT, sizeof(float));
	VAO_quad.Push(2, GL_FLOAT, sizeof(float));
	VAO_quad.AddLayout();
	VBO_quad.Unbind();
	IBO_quad.Bind();

	GLCall(glDrawElements(GL_TRIANGLES, IBO_quad.GetCount(), GL_UNSIGNED_INT, nullptr));
	VAO_quad.Unbind();
}

void Renderer::InitPrimitiveModels()
{
}
