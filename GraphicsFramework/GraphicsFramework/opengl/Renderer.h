/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "GLEW/include/GL/glew.h"
#include "../core/Singleton.h"
#include "SDL/include/SDL.h"
#include <vector>
#include <glm/mat4x2.hpp>

class SDL_Window;
class Camera;

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x,__FILE__,__LINE__))


void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);

class VertexArray;
class ElementArrayBuffer;
class Shader;
class Object;

struct DebugData
{
	VertexArray* mVAO;
	ElementArrayBuffer* mEBO;
	glm::mat4 mModelMatrix;
	GLuint mDrawCommand;
	glm::vec3 mColor;

	DebugData(VertexArray* _v, ElementArrayBuffer* _e, glm::mat4 _m, GLuint _d, glm::vec3 _c):
	mVAO(_v), mEBO(_e),mModelMatrix(_m),mDrawCommand(_d), mColor(_c) {}
};

class Transform;
class Shape;
class Material;

struct RenderData
{
	Transform* mTransform;
	Shape* mShape;
	Material* mMaterial;

	void Clear() { mTransform = nullptr; mShape = nullptr; mMaterial = nullptr; }
};

class Renderer
{
	SINGLETON(Renderer)
public:
	void Init(SDL_Window* pWindow);
	void Clear() const;
	void SwapBuffers();
	 
	void Draw(const VertexArray& va, const ElementArrayBuffer& ib, const Shader& shader) const;
	void DebugDraw(const VertexArray& va, const ElementArrayBuffer& ib, const Shader& shader) const;
	void DebugDrawLines(const VertexArray & va, const ElementArrayBuffer& ib, const Shader & shader) const;
	void DrawDebugCircle(const VertexArray& va, const Shader& shader) const;
	void DrawDebugLine(const VertexArray& va, const Shader& shader) const;
	void DrawQuad();
	void DrawDebugObjects(Shader* shader);
	void DrawAllObjects(Shader* shader);
	void AddDebugRenderData(DebugData data);
	void SetClearColor(glm::vec3 color) { mClearColor = color; }
	RenderData& GetRenderData() { return mCurrentRenderData; }
	void AddRenderData();

private:
	SDL_Window* pWindow;
	std::vector<DebugData> mDebugRenderData;
	std::vector<RenderData> mRenderData;
	RenderData mCurrentRenderData;
	glm::vec3 mClearColor;
};