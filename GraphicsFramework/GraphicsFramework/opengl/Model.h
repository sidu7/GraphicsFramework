#pragma once
#include <string>
#include <vector>
#include "Mesh.h"
#include "Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class Shader;

class Model 
{
 public:
	 void LoadModel(std::string path);
	 void Draw(Shader* shader);
 private:
	 std::vector<Mesh> mMeshes;
	 std::vector<Mesh::TextureData> loadedTextures;
	 std::string directory;
	 void ProcessNode(aiNode* node, const aiScene* scene);
	 Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
	 std::vector<Mesh::TextureData> LoadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);
};