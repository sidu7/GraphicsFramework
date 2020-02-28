#pragma once
#include "Singleton.h"
#include <unordered_map>
#include "glm/glm.hpp"

class VertexArray;
class ElementArrayBuffer;

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


class ShapeManager
{
	SINGLETON(ShapeManager)
public:	
	void Init();
	void Close();
	std::vector<std::pair<VertexArray*, ElementArrayBuffer*>> mShapes;

private:
	void MakeVAO(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Shapes shape);
};