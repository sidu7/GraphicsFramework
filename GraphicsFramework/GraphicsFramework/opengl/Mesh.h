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
		glm::ivec4 BoneIndex;
		glm::vec4 BoneWeights;

		void AddBone(int index, int boneindex, float weight)
		{
			if (index > 3)
			{
				float min = glm::min(BoneWeights[0], glm::min(BoneWeights[1], glm::min(BoneWeights[2], BoneWeights[3])));
				if (BoneWeights[0] == min) { BoneWeights[0] = weight, BoneIndex[0] = boneindex; }
				if (BoneWeights[1] == min) { BoneWeights[1] = weight, BoneIndex[1] = boneindex; }
				if (BoneWeights[2] == min) { BoneWeights[2] = weight, BoneIndex[2] = boneindex; }
				if (BoneWeights[3] == min) { BoneWeights[3] = weight, BoneIndex[3] = boneindex; }
				return;
			}
			BoneIndex[index] = boneindex;
			BoneWeights[index] = weight;
		}
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