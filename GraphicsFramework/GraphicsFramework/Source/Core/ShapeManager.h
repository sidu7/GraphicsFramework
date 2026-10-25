#pragma once
#include "Singleton.h"
#include <unordered_map>
#include "glm/glm.hpp"

class IndexBuffer;
class VertexBuffer;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

enum Shapes
{
	QUAD,
	SPHERE,
	CUBE,
	TEAPOT,
	WIRECUBE,
	PYRAMID,
	LINE,
	SHAPES_NUM
};

struct ShapeData
{
	IndexBuffer* mIBO = nullptr;
	VertexBuffer* mVBO = nullptr;
};

class ShapeManager : public Singleton<ShapeManager>
{
public:
	ShapeManager() = default;
	~ShapeManager();

	// Prevent double-initialization
	void Init();
	void Close();

	std::vector<ShapeData> mShapes;

private:
	// Per-shape helpers
	void CreateLine();
	void CreateWireCube();
	void CreatePyramid();
	void CreateQuad();
	void CreateCube();
	void CreateSphere();
	void CreateTeapot();

	// Shared VAO creation (takes data by const reference to avoid copies)
	void MakeVAO(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, Shapes shape);

	bool mInitialized = false;
};