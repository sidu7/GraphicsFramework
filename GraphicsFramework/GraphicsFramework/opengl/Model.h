#pragma once
#include <string>
#include <vector>
#include "Mesh.h"
#include "Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <unordered_map>

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
	glm::mat4 mCurrentTransformation;
	glm::mat4 mOffset;
	bool isAnimated;
	std::string mName;
	std::unordered_map<std::string, AnimationData> mAnimations;
};

class Model 
{
 public:
	 void LoadModel(std::string path);
	 void Draw(Shader* shader);
private:
	 void ProcessNode(aiNode* node, const aiScene* scene, Bone* parent = nullptr);
	 Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
	 std::vector<Mesh::TextureData> LoadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);
	 inline unsigned int GetBoneIndex(std::string name)
	 {
		 for (unsigned int i = 0; i < mBones.size(); ++i)
		 {
			 if (mBones[i].mName == name)
			 {
				 return mBones[i].mIndex;
			 }
		 }
	 }
	 void ProcessAnimationData(const aiScene* scene);

public:
	 std::vector<Bone> mBones;
 private:
	 std::vector<Mesh> mMeshes;
	 std::vector<Mesh::TextureData> loadedTextures;
	 std::string directory;
	 unsigned int mBoneCount;
};

inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
	glm::mat4 to;


	to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
	to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
	to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
	to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

	return to;
}

inline glm::vec3 aiVec3toGlm(const aiVector3D* from)
{
	return glm::vec3(from->x, from->y, from->z);
}