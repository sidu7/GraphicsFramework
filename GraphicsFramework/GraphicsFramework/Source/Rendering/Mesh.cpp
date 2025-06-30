#include "Mesh.h"
#include "Shader.h"
#include <string>
#include "Rendering/RenderingFactory.h"

Mesh::Mesh(std::vector<VertexData> vertices, std::vector<TextureData> textures, std::vector<uint16_t> indices) :
	VBO(nullptr),
	IBO(nullptr)
{
	mVertices = vertices;
	mTextures = textures;
	mIndices = indices;
	SetupMesh();
}

void Mesh::Destroy()
{
	delete VBO;
	delete IBO;
}

void Mesh::Draw(Shader * shader)
{
	unsigned int diffuseNum = 1;
	unsigned int specularNum = 1;
	unsigned int normalNum = 1;
	unsigned int heightNum = 1;
	for (unsigned int i = 0; i < mTextures.size(); ++i)
	{		
		std::string number;
		std::string name;
		if (name == DIFFUSE)
		{
			number = std::to_string(diffuseNum++);
		}
		else if (name == SPECULAR)
		{
			number = std::to_string(specularNum++);
		}
		else if (name == NORMAL)
		{
			number = std::to_string(normalNum++);
		}
		else if (name == HEIGHT)
		{
			number = std::to_string(heightNum++);
		}

		//shader->SetUniform1i(name + number, i);

		Renderer::Instance()->BindTexture(mTextures[i].texture, i);
	}

	Renderer::Instance()->Draw(VBO, IBO, shader);

	for (unsigned int i = 0; i < mTextures.size(); ++i)
	{
		Renderer::Instance()->UnbindTexture(mTextures[i].texture, i);
	}
}

void Mesh::SetupMesh()
{
	VBO = RenderingFactory::Instance()->CreateVertexBuffer();
	IBO = RenderingFactory::Instance()->CreateIndexBuffer();

	Renderer* pRenderer = Renderer::Instance();
	
	VBO->SetDataLayout(
		{
			VertexFormat::Vec3,  // aPos
			VertexFormat::Vec3,  // aNormal
			VertexFormat::Vec2,  // aTexCoord
			VertexFormat::iVec4, // aBoneIDs
			VertexFormat::Vec4 	 // aWeights
		});

	VBO->AddData(&mVertices[0], mVertices.size() * sizeof(VertexData));	   
	IBO->AddData(&mIndices[0], mIndices.size(), IndexType::UInt16);
}
