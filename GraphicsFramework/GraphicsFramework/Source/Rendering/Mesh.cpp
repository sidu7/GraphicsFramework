#include "Mesh.h"
#include "Shader.h"
#include <string>
#include "Rendering/RenderingFactory.h"

Mesh::Mesh(std::vector<VertexData> vertices, std::vector<TextureData> textures, std::vector<unsigned int> indices) :
	VAO(nullptr),
	VBO(nullptr),
	EBO(nullptr)
{
	mVertices = vertices;
	mTextures = textures;
	mIndices = indices;
	SetupMesh();
}

void Mesh::Destroy()
{
	delete VAO;
	delete VBO;
	delete EBO;
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

		shader->SetUniform1i(name + number, i);

		Renderer::Instance()->BindTexture(mTextures[i].texture, i);
	}

	Renderer::Instance()->BindVertexArray(VAO);
	GLCall(glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0));
	Renderer::Instance()->UnbindVertexArray(VAO);

	GLCall(glActiveTexture(GL_TEXTURE0));
}

void Mesh::SetupMesh()
{
	VAO = RenderingFactory::Instance()->CreateVertexArray();
	VBO = RenderingFactory::Instance()->CreateVertexBuffer();
	EBO = RenderingFactory::Instance()->CreateElementArrayBuffer();

	Renderer* pRenderer = Renderer::Instance();

	pRenderer->BindVertexArray(VAO);
	pRenderer->BindVertexBuffer(VBO);
	   
	VBO->AddData(&mVertices[0], mVertices.size() * sizeof(VertexData));
	   
	pRenderer->BindElementArrayBuffer(EBO);
	EBO->AddData(&mIndices[0], mIndices.size(), sizeof(unsigned int));
	   
	VAO->Push(3, GL_FLOAT, sizeof(float));
	VAO->Push(3, GL_FLOAT, sizeof(float));
	VAO->Push(2, GL_FLOAT, sizeof(float));
	VAO->Push(4, GL_INT,   sizeof(int));
	VAO->Push(4, GL_FLOAT, sizeof(float));
	VAO->AddLayout();
	   
	pRenderer->UnbindVertexArray(VAO);
	pRenderer->UnbindVertexBuffer(VBO);
	pRenderer->UnbindElementArrayBuffer(EBO);
}
