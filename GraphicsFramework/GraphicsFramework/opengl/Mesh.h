#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Texture.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementArrayBuffer.h"
#include "Quaternion.h"

#define DIFFUSE "texture_diffuse"
#define SPECULAR "texture_specular"
#define NORMAL "texture_normal"
#define HEIGHT "texture_height"

class Shader;

class Mesh
{
public:
	struct VertexData
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		//glm::ivec4 BoneIndex;
		//glm::vec4 BoneWeights;
	};

	struct TextureData
	{
		Texture* texture;
		std::string type;
	};

public:
	std::vector<VertexData> mVertices;
	std::vector<TextureData> mTextures;
	std::vector<unsigned int> mIndices;

	Mesh(std::vector<VertexData> vertices, std::vector<TextureData> textures, std::vector<unsigned int>indices);

	void Draw(Shader* shader);

private:
	VertexArray VAO;
	VertexBuffer VBO;
	ElementArrayBuffer EBO;

	void SetupMesh();
};