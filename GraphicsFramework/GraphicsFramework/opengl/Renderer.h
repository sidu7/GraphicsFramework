/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "GL/glew.h"
#include "../src/Singleton.h"
#include "SDL.h"

class SDL_Window;
class Camera;

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x,__FILE__,__LINE__))


void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);

class VertexArray;
class IndexBuffer;
class Shader;

class Renderer
{
	SINGLETON(Renderer)
public:
	void Init(SDL_Window* pWindow);
	void Clear() const;
	void SwapBuffers() const;

	void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;
	void DebugDraw(const VertexArray & va, const IndexBuffer & ib, const Shader & shader) const;
	void DrawDebugCircle(const VertexArray& va, const Shader& shader) const;
	void DrawDebugLine(const VertexArray& va, const Shader& shader) const;
	void DrawQuad();

private:
	void InitPrimitiveModels();

private:
	SDL_Window* pWindow;

public:
	Camera* pCamera;	
};