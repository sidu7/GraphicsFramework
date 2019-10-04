#pragma once
#include "Singleton.h"
#include "../opengl/Model.h"
#include "../opengl/Shader.h"
#include "Light.h"

class AnimationScene
{
	SINGLETON(AnimationScene)
public:
	void Init();
	void Draw();
private:
	Model demoModel;

	Light* light;

	Shader* modelDraw;
};