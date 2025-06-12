#pragma once
#include <core/Scene.h>
#include <glm/glm.hpp>
#include <vector>
#include "HeightField.h"

class Light;
class Shader;
class VertexArray;
class VertexBuffer;
class ElementArrayBuffer;

class Wave : public Scene
{
public:
	~Wave() override;
	void Init() override;
	void Update() override;
	void DebugDisplay() override;

private:
	Light* mLight;

	Shader* mShader;

	HeightField mHeightMap;
	
	VertexArray* mVAO;
	VertexBuffer* mVBOs[2];
	enum { VERT = 0, NORM = 1 };
	ElementArrayBuffer* mEBO;
	int mTriCount;
	std::vector<glm::vec3> mMeshPositions, mMeshNormals;
	glm::vec3 mDiffuseColor;
	float mWaveIntensity;
};