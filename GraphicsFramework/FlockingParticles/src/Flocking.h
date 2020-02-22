#pragma once
#include "core/Scene.h"
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Light;
class Shader;
class Object;
class Obstacle;

class Flocking : public Scene
{
public:
	~Flocking();
	void Init() override;
	void Update() override;
	void DebugDisplay() override;

private:
	std::vector<Object*> FindNeighbours(Object* fish);
	void FirstandThirdAcceleration(std::vector<Object*>& neighbours, glm::vec3& posA,
		glm::vec3& a_first, glm::vec3& a_third);
	void AvoidObstacles(glm::vec3& a_first, Object* fish);
	glm::vec3 SecondAcceleration(std::vector<Object*>& neighbours, glm::vec3 velA);
	glm::vec3 PrioritizedAcceleration(const glm::vec3* accels);

private:
	Light* mLight;

	Shader* mShader;

	Object* Boundary;
	std::vector<Object*> mFishes;
	std::vector<Obstacle*> mObstacles;
	
	float mFishViewAngle;
	float mFishViewDistance;
	float mBoidTightness;
	float mMaxAcceleration;
	float mVelocityAttainTime;
};