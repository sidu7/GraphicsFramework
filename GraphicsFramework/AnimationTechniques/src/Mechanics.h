#pragma once
#include "core/Scene.h"

#include <glm/glm.hpp>
#include <utility>
#include <vector>

class Light;
class Shader;
class VertexArray;
class ElementArrayBuffer;
struct ShapeData;

struct Mass
{
	float mMass;
	glm::vec3 mVelocity;
	glm::vec3 mForce;
};

struct RopeNode
{
	Mass mBody;
	glm::vec3 mPosition;
	glm::vec3 mRotation;
	glm::vec3 mScale;
	glm::mat4 mModel;
	ShapeData* mRenderData;
};

struct Spring
{
	RopeNode* mMass1;
	RopeNode* mMass2;
};

struct State
{
	glm::vec3 mPosition;
	glm::vec3 mVelocity;
};

struct Derivative
{
	glm::vec3 mDPosition;
	glm::vec3 mDVelocity;
};

class Mechanics : public Scene
{
public:
	~Mechanics();
	void Init() override;
	void Update() override;
	void DebugDisplay() override;

private:
	void Simulate(float deltaTime);
	void Start();
	void Solve();
	void Apply(float deltaTime);

	Derivative Func(const State& initial, float time, const Derivative& x, glm::vec3 acceleration);
	glm::vec3 Func(glm::vec3 initial, float time, glm::vec3 x);

private:
	Light* mLight;
	Shader* Drawing;

	float Time;
	int NodeCount;
	float NodeMass;
	std::vector<RopeNode> mRopeNodes;
	std::vector<Spring> mSprings;
	float mSpringConstant;
	float mSpringLength;
	float mSpringFriction;
	glm::vec3 mGravitationalForce;
	float mAirFriction;
	glm::vec3 mRopeConnectionPosition1;
	glm::vec3 mRopeConnectionPosition2;
	float mRopeConnectionVelocity;
};