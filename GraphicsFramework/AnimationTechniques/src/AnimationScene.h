#pragma once
#include "Core/Scene.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <stack>
#include <utility>
#include <algorithm>

// Application-local shader IDs (shared across all scenes in this project)
#include "ProjectShaderIDs.h"

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
class UniformBuffer;
class VertexBuffer;
class IndexBuffer;

enum class Mode
{
	Animation,
	IK
};

// App-local material block. 64 bytes so it stays 16-byte aligned; the extra
// ints are padding, not flags. Must match res/Shaders/*.frag exactly.
struct AnimationMaterialUBO
{
	glm::vec4 mDiffuse;
	glm::vec4 mSpecular;
	float mShininess;
	int32_t mLighting;
	int32_t mUseTexture;
	int32_t mPad0;
	int32_t mPad1;
	int32_t mPad2;
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
	void IKUpdate(glm::mat4 model, float time);
	inline glm::mat4 RotationFromDirection(const glm::vec3& direction);
	void UploadBones();
	void BuildCurveBuffers();
	void BuildBonesBuffers();

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

	// Per-draw UBO pool — each draw needs its own UBOs so Vulkan writes don't
	// bleed across draws in the same frame.
	struct DrawUniforms
	{
		UniformBuffer* mObjectUBO = nullptr;
		UniformBuffer* mMaterialUBO = nullptr;
	};

	DrawUniforms& NextDrawUniforms();

	Model* demoModel = nullptr;
	std::string animation;
	bool drawModel = true, drawBones = false;

	glm::vec3 mGoalPosition;

	Light* light = nullptr;

	Shader* modelDrawShader = nullptr;
	Shader* drawingShader = nullptr;

	// UBOs
	UniformBuffer* mCameraUBO = nullptr;
	UniformBuffer* mLightUBO = nullptr;
	UniformBuffer* mBoneUBO = nullptr;
	std::vector<DrawUniforms> mDrawUniforms;
	size_t mDrawIndex = 0;

	// Animation / IK state (kept from the old implementation)
	std::vector<glm::mat4> mBonesTransformation;
	double AnimationRunTime = 0.0;
	bool isPaused = false;
	float mSpeed = 18.0f;
	glm::mat4 mPathMatrix;
	float PathRunTime = 0.0f;
	float totalRunTime = 0.0f;
	float t1 = 0.0f, t2 = 0.0f;
	Mode mMode = Mode::Animation;

	// Curve (path walking)
	bool mPathWalk = false;
	bool showControlWindow = false;
	std::vector<glm::vec4> controlPoints;
	glm::mat4 CurveMatrix;
	std::vector<glm::mat4> controlPointsMatrices;
	unsigned int mCurvePointsSize = 0;
	float mTolerance = 0.0001f;
	// Table has entries in the form (distance, parameter of curve, curve index)
	std::vector<TABLE_ENTRY> mArcLengthTable;

	// IK
	bool mDoOnce = true;
	std::vector<unsigned int> mIKBones;
	glm::vec3 mEndEffector;
	float IKDuration = 1.0f;
	int maxIterations = 10;
	float sqrDistanceError = 0.02f;
	glm::vec3 mLocalGoalPosition;

	// Curve polyline (RHI)
	VertexBuffer* mCurveVBO = nullptr;
	IndexBuffer* mCurveIBO = nullptr;

	// Bone debug lines (RHI)
	VertexBuffer* mBonesVBO = nullptr;
};
