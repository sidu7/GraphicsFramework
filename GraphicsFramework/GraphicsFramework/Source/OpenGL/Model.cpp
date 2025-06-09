#pragma once

#include "Core/Core.h"
#include "Model.h"
#include "GLEW/include/GL/glew.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"
#include <iostream>
#include "assimp/include/assimp/cimport.h"
#include "assimp/include/assimp/version.h"

void Model::Draw(Shader* shader)
{
	for (unsigned int i = 0; i < mMeshes.size(); ++i)
	{
		mMeshes[i].Draw(shader);
	}
}

void Model::LoadModel(std::string path)
{
	int MajorVer = aiGetVersionMajor();
	int MinorVer = aiGetVersionMinor();
	std::cout << "Assimp Ver " << MajorVer << "." << MinorVer << std::endl;
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	}
	directory = path.substr(0, path.find_last_of('/'));

	mBoneCount = 0;

	ProcessNode(scene->mRootNode, scene);

	ProcessAnimationData(scene, path.substr(path.find_last_of('/') + 1, path.size() - 8));
}
void Model::AddAnimation(std::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	}

	ProcessAnimationData(scene, path.substr(path.find_last_of('/') + 1, path.size() - 8));
}

DISABLE_OPTIMIZATION
void Model::ProcessAnimationData(const aiScene* scene, std::string name)
{
	for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
	{
		aiAnimation* animation = scene->mAnimations[i];
		for (unsigned int j = 0; j < animation->mNumChannels; ++j)
		{
			unsigned int index = GetBoneIndex(animation->mChannels[j]->mNodeName.data);
			AnimationData animData;
			animData.mDuration = animation->mDuration;
			animData.mTicksPerSec = animation->mTicksPerSecond;
			//Key Positions
			for (unsigned int k = 0; k < animation->mChannels[j]->mNumPositionKeys; ++k)
			{
				animData.mKeyPositions.push_back(std::make_pair(animation->mChannels[j]->mPositionKeys[k].mTime, aiVec3toGlm(&animation->mChannels[j]->mPositionKeys[k].mValue)));
			}

			//Key Rotations
			for (unsigned int k = 0; k < animation->mChannels[j]->mNumRotationKeys; ++k)
			{
				aiQuaternion aiquat = animation->mChannels[j]->mRotationKeys[k].mValue;
				Quaternion quat(aiquat.x, aiquat.y, aiquat.z, aiquat.w);
				animData.mKeyRotations.push_back(std::make_pair(animation->mChannels[j]->mRotationKeys[k].mTime, quat));
			}

			//Key Scalings
			for (unsigned int k = 0; k < animation->mChannels[j]->mNumScalingKeys; ++k)
			{
				animData.mKeyScalings.push_back(std::make_pair(animation->mChannels[j]->mScalingKeys[k].mTime, aiVec3toGlm(&animation->mChannels[j]->mScalingKeys[k].mValue)));
			}

			mBones[index].mAnimations[name] = animData;
			mBones[index].isAnimated = true;
		}
		mAnimations.push_back(name);
	}
}
ENABLE_OPTIMIZATION

void Model::ProcessNode(aiNode* node, const aiScene* scene, Bone* parent)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mMeshes.push_back(ProcessMesh(mesh, scene));
	}

	Bone child;
	child.mIndex = mBoneCount++;
	child.mTransformation = *(glm::mat4*)(&node->mTransformation.Transpose());
	child.mName = node->mName.data;
	child.isAnimated = false;
	child.mOffset = glm::mat4(1.0f);
	if (parent)
	{
		child.mParentIndex = parent->mIndex;
	}
	else
	{
		child.mParentIndex = -1;
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
	std::vector<unsigned int> vertexindex;

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

		vertex.BoneIndex = glm::ivec4(0);
		vertex.BoneWeights = glm::vec4(0.0f);

		vertices.push_back(vertex);
		vertexindex.push_back(0);
	}

	//Processing textures
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Mesh::TextureData> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, DIFFUSE, scene);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<Mesh::TextureData> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, SPECULAR, scene);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		std::vector<Mesh::TextureData> normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS, NORMAL, scene);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		std::vector<Mesh::TextureData> heightMaps = LoadMaterialTextures(material, aiTextureType_HEIGHT, HEIGHT, scene);
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

	//Processing Bones
	for (unsigned int i = 0; i < mesh->mNumBones; ++i)
	{
		aiBone* bone = mesh->mBones[i];
		unsigned int boneIndex = GetBoneIndex(bone->mName.data);
		for (unsigned int j = 0; j < bone->mNumWeights; ++j)
		{
			if (bone->mWeights)
			{
				unsigned int index = bone->mWeights[j].mVertexId;
				vertices[index].BoneIndex[vertexindex[index]] = boneIndex;
				vertices[index].BoneWeights[vertexindex[index]++] = bone->mWeights[j].mWeight;
			}
		}
		mBones[boneIndex].mOffset = *(glm::mat4*)(&bone->mOffsetMatrix.Transpose());
	}

	return Mesh(vertices, textures, indices);
}

std::vector<Mesh::TextureData> Model::LoadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName, const aiScene* scene)
{
	std::vector<Mesh::TextureData> textures;

	for (unsigned int i = 0; i < material->GetTextureCount(type); ++i)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		bool skip = false;
		//Check in cache
		for (unsigned int j = 0; j < loadedTextures.size(); ++j)
		{
			if (std::strcmp(loadedTextures[i].texture->mFilePath.c_str(), str.C_Str()) == 0)
			{
				textures.push_back(loadedTextures[j]);
				skip = true;
				break;
			}
		}
		//Not in cache load it
		if (!skip)
		{
			Mesh::TextureData data;
			const aiTexture* tex = scene->GetEmbeddedTexture(str.C_Str());
			if (tex && tex->mHeight == 0)
			{
				data.texture = new Texture(tex->pcData, tex->mWidth);
				data.type = typeName;
				textures.push_back(data);
				loadedTextures.push_back(data);
			}
			else
			{
				std::string fileName = str.C_Str();
				data.texture = new Texture(directory + '/' + fileName);
				data.type = typeName;
				textures.push_back(data);
				loadedTextures.push_back(data);
			}
		}
	}

	return textures;
}


