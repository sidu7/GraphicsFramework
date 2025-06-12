#pragma once

#include "Singleton.h"
#include "Core.h"

class Camera;
class Scene;
class Window;

class Engine : public Singleton<Engine>
{
public:
	
	void Start(Scene* scene,int width, int height);
	void Run();
	void Stop();

	bool appIsRunning;
	bool stopMoving;
	bool mPause;

	Camera* GetCamera();
	inline Scene* GetScene() { return pScene; }
protected:
	Camera* pCamera;
	Scene* pScene;
};