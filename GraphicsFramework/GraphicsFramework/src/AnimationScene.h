#pragma once
#include "Singleton.h"
#include "../opengl/Model.h"
#include "../opengl/Shader.h"
#include "Light.h"

struct BoneRenderData
{
	glm::mat4 transformation;
};

class AnimationScene
{
	SINGLETON(AnimationScene)
public:
	void Init();
	void Draw();

private:
	void AnimatorUpdate(std::string animName);
	VertexArray CreateBonesData();
	template<typename T>
	unsigned int FindLessThaninList(double time, std::vector<std::pair<double,T>> list)
	{
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			if (list[i].first >= time)
			{
				return i;
			}
		}
	}

private:
	Model demoModel; 

	Light* light;

	Shader* modelDraw;

	Shader* Drawing;

	std::vector<glm::mat4> mBonesTransformation;
	double RunTime;
	bool isPaused;
};