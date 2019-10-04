#include "Mesh.h"
#include "Shader.h"
#include <string>

Mesh::Mesh(std::vector<VertexData> vertices, std::vector<TextureData> textures, std::vector<unsigned int> indices)
{
	mVertices = vertices;
	mTextures = textures;
	mIndices = indices;
	SetupMesh();
}

void Mesh::Draw(Shader * shader)
{
	unsigned int diffuseNum = 1;
	unsigned int specularNum = 1;
	unsigned int normalNum = 1;
	unsigned int heightNum = 1;
	/*for (unsigned int i = 0; i < mTextures.size(); ++i)
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

		shader->SetUniform1i("Material." + name + number, i);

		mTextures[i].texture->Bind(i);
	}*/

	VAO.Bind();
	GLCall(glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0));
	VAO.Unbind();

	GLCall(glActiveTexture(GL_TEXTURE0));
}

void Mesh::SetupMesh()
{
	VAO.Bind();
	VBO.Bind();

	VBO.AddData(&mVertices[0], mVertices.size() * sizeof(VertexData));

	EBO.Bind();
	EBO.AddData(&mIndices[0], mIndices.size(), sizeof(unsigned int));

	VAO.Push(3, GL_FLOAT, sizeof(float));
	VAO.Push(3, GL_FLOAT, sizeof(float));
	VAO.Push(2, GL_FLOAT, sizeof(float));
	VAO.AddLayout();

	VAO.Unbind();
}
