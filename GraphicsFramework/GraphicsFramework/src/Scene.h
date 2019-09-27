#pragma once
#include "Singleton.h"
#include "../opengl/Shader.h"
#include "../opengl/Model.h"
#include "../opengl/FrameBuffer.h"
#include "Light.h"

class Scene
{
	SINGLETON(Scene)
public:
	void Init();
	void Draw();

private:
	Shader* shader;
	Shader* lighting;
	Shader* ambient;
	Model demoModel;

	Light* light;
	FrameBuffer* G_Buffer;
	int gBuffershow;

};