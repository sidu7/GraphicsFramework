#pragma once
#include <string>
#include <vector>
#include "Assets/Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>
#include <iostream>

class Shader;

struct AnimationData
{
	std::vector<std::pair<double, glm::vec3>> mKeyPositions;
	std::vector<std::pair<double, Quaternion>> mKeyRotations;
	std::vector<std::pair<double, glm::vec3>> mKeyScalings;
	double mDuration;
	double mTicksPerSec;
};

struct Bone
{
	unsigned int mIndex;
	unsigned int mParentIndex;
	glm::mat4 mTransformation;
	glm::mat4 mCurrentLocalTransformation;
	glm::mat4 mCurrentGlobalTransformation;
	glm::mat4 mOffset;
	glm::mat4 mIKTransformation;
	bool isAnimated;
	std::string mName;
	std::unordered_map<std::string, AnimationData> mAnimations;
};

class Model 
{
 public:
	 ~Model();

	 void LoadModel(std::string path);
	 void AddAnimation(std::string path);
	 void Draw(Shader* shader);
private:
	 void ProcessNode(aiNode* node, const aiScene* scene, Bone* parent = nullptr);
	 Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
	 std::vector<Mesh::TextureData> LoadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName,const aiScene* scene);
	 inline unsigned int GetBoneIndex(std::string name)
	 {
		 for (unsigned int i = 0; i < mBones.size(); ++i)
		 {
			 if (mBones[i].mName == name)
			 {
				 return mBones[i].mIndex;
			 }
		 }
		 std::cout << "Bone index not found" << std::endl;
		 return -1;
	 }
	 void ProcessAnimationData(const aiScene* scene, std::string name);

public:
	 std::vector<Bone> mBones;
	 std::vector<std::string> mAnimations;
 private:
	 std::vector<Mesh> mMeshes;
	 std::vector<Mesh::TextureData> loadedTextures;
	 std::string directory;
	 unsigned int mBoneCount;
};

inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
	glm::mat4 to;


	to[0][0] = (float)from->a1; to[0][1] = (float)from->a2;  to[0][2] = (float)from->a3; to[0][3] = (float)from->a4;
	to[1][0] = (float)from->b1; to[1][1] = (float)from->b2;  to[1][2] = (float)from->b3; to[1][3] = (float)from->b4;
	to[2][0] = (float)from->c1; to[2][1] = (float)from->c2;  to[2][2] = (float)from->c3; to[2][3] = (float)from->c4;
	to[3][0] = (float)from->d1; to[3][1] = (float)from->d2;  to[3][2] = (float)from->d3; to[3][3] = (float)from->d4;

	return to;
}

inline glm::mat4 aiMatrix3x3ToGlm(const aiMatrix3x3* from)
{
	glm::mat4 to =  glm::mat4(1.0f);


	to[0][0] = (float)from->a1; to[0][1] = (float)from->a2;  to[0][2] = (float)from->a3;
	to[1][0] = (float)from->b1; to[1][1] = (float)from->b2;  to[1][2] = (float)from->b3;
	to[2][0] = (float)from->c1; to[2][1] = (float)from->c2;  to[2][2] = (float)from->c3;
	

	return to;
}

inline glm::vec3 aiVec3toGlm(const aiVector3D* from)
{
	return glm::vec3(from->x, from->y, from->z);
}