#include "Model.h"
#include <iostream>

void Model::Draw(Shader* shader)
{
	for (unsigned int i = 0; i < mMeshes.size(); ++i)
	{
		mMeshes[i].Draw(shader);
	}
}

void Model::ProcessNode(aiNode* node, const aiScene* scene, Bone* parent)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mMeshes.push_back(ProcessMesh(mesh, scene));
	}

	Bone child;
	child.mIndex = mBoneCount++;
	child.mTransformation = aiMatrix4x4ToGlm(&node->mTransformation);
	child.mName = node->mName.data;
	if (parent)
	{
		child.mParentIndex = parent->mIndex;
	}
	mBones.push_back(child);

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene, &child);
	}
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Mesh::VertexData> vertices;
	std::vector<Mesh::TextureData> textures;
	std::vector<unsigned int> indices;

	//Processing vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		Mesh::VertexData vertex;

		vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		
		if (mesh->mTextureCoords[0])
		{
			vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}
		else
		{
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		}

		vertices.push_back(vertex);
	}

	//Processing textures
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Mesh::TextureData> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, DIFFUSE);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<Mesh::TextureData> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, SPECULAR);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		std::vector<Mesh::TextureData> normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS, NORMAL);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		std::vector<Mesh::TextureData> heightMaps = LoadMaterialTextures(material, aiTextureType_HEIGHT, HEIGHT);
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
	}

	//Processing indices
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	return Mesh(vertices, textures, indices);
}

std::vector<Mesh::TextureData> Model::LoadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName)
{
	std::vector<Mesh::TextureData> textures;

	for (unsigned int i = 0; i < material->GetTextureCount(type); ++i)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < loadedTextures.size(); ++j)
		{
			if (std::strcmp(loadedTextures[i].texture->mFilePath.c_str(), str.C_Str()) == 0)
			{
				textures.push_back(loadedTextures[j]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{
			Mesh::TextureData data;
			std::string fileName = str.C_Str();
			std::string name = fileName.substr(fileName.find_last_of('\\') + 1);
			data.texture = new Texture(directory + '/' + name);
			data.type = typeName;
			textures.push_back(data);
			loadedTextures.push_back(data);
		}
	}

	return textures;
}

void Model::LoadModel(std::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	}
	directory = path.substr(0, path.find_last_of('/'));

	mBoneCount = 0;

	ProcessNode(scene->mRootNode, scene);
}
