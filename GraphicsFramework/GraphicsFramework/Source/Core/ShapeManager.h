#pragma once
#include "Singleton.h"
#include <unordered_map>
#include "glm/glm.hpp"

class VertexArray;
class ElementArrayBuffer;
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
	VertexArray* m_VAO = nullptr;
	ElementArrayBuffer* m_EBO = nullptr;
	VertexBuffer* m_VBO = nullptr;
};

class ShapeManager : public Singleton<ShapeManager>
{
public:	
	void Init();
	void Close();
	std::vector<ShapeData> mShapes;

private:
	void MakeVAO(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Shapes shape);
};