#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Rendering/Texture.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/IndexBuffer.h"
#include "Utils/Math/Quaternion.h"

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
		glm::ivec4 BoneIndex;
		glm::vec4 BoneWeights;
	};

	struct TextureData
	{
		Texture* texture;
		std::string type;
	};

public:
	std::vector<VertexData> mVertices;
	std::vector<TextureData> mTextures;
	std::vector<uint16_t> mIndices;

	Mesh(std::vector<VertexData> vertices, std::vector<TextureData> textures, std::vector<uint16_t>indices);
	
	void Destroy();
	void Draw(Shader* shader);

private:
	VertexBuffer* VBO;
	IndexBuffer* IBO;

	void SetupMesh();
};