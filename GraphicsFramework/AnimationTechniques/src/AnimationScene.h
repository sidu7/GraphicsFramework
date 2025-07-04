#pragma once
#include "Core/Scene.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

#define MAKE_TUPLE(x,y,z) std::make_pair(x,std::make_pair(y,z))
#define TABLE_ENTRY std::pair<float, std::pair<float, int>>

struct BoneRenderData
{
	glm::mat4 transformation;
};

class VertexArray;
class Model;
class Shader;
class Light;

enum class Mode
{
	Animation,
	IK
};

class AnimationScene : public Scene
{
public:
	virtual ~AnimationScene();
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void RenderObject(class Object* object, class Shader* shader) override;
	virtual void DebugDisplay() override;

private:
	void AnimatorUpdate(std::string animName);
	inline glm::mat4 RotationFromDirection(const glm::vec3& direction);
	void DrawBones();
	template<typename T>
	inline unsigned int FindLessThaninList(double time, std::vector<std::pair<double, T>> list)
	{
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			if (list[i].first >= time)
			{
				return i;
			}
		}

		return 0;
	}
	inline glm::vec4 GetPointOnCurve(float t, glm::mat4& controlPointMatrix);
	void RecalculateMatrices();
	void CreateAxisLengthTable();
	inline std::pair<float, int> SearchInTable(float distance);
	glm::vec4 GetDerivativeOfPointOnCurve(float t, glm::mat4& controlPointMatrix);
	void Deserialize();
	void CreateControlPointsVAO();
	void IKUpdate(glm::mat4 model, float time);

private:
	Model* demoModel;
	std::string animation;
	bool drawModel, drawBones;

	glm::vec3 mGoalPosition;

	Light* light;

	Shader* modelDraw;

	Shader* Drawing;

	std::vector<glm::mat4> mBonesTransformation;
	double AnimationRunTime;
	bool isPaused;
	// Model data
	float mSpeed;
	glm::mat4 mPathMatrix;
	float PathRunTime;
	float totalRunTime;
	float t1, t2;
	Mode mMode;

	//Curve
	bool mPathWalk;
	bool showControlWindow;
	std::vector<glm::vec4> controlPoints;
	glm::mat4 CurveMatrix;
	std::vector<glm::mat4> controlPointsMatrices;
	VertexArray* mCurveVAO;
	unsigned int mCurvePointsSize;
	float mTolerance;
	// Table has entries in the form (distance, parameter of curve, curve index)
	std::vector<TABLE_ENTRY> mArcLengthTable;

	//IK
	bool mDoOnce;
	std::vector<unsigned int> mIKBones;
	glm::vec3 mEndEffector;
	float IKDuration;
	int maxIterations;
	float sqrDistanceError;
	glm::vec3 mLocalGoalPosition;
};