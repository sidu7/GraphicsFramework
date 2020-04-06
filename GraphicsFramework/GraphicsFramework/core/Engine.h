#pragma once

#include <SDL/include/SDL.h>
#include <glm/glm.hpp>
#include "Singleton.h"

class Camera;
class Scene;

class Engine
{
	SINGLETON(Engine);
public:
	
	void Start(Scene* scene,int width, int height);
	void Run();
	void Stop();

	glm::vec2 GetWindowSize();

	bool appIsRunning;
	bool stopMoving;
	bool mPause;

	SDL_Window* pWindow;
	SDL_GLContext glContext;
	int scrWidth, scrHeight;

	Camera* pCamera;
	Scene* pScene;
};