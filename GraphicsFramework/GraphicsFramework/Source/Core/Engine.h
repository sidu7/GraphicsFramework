#pragma once

#include "Core/Core.h"

class Camera;
class Scene;
class Window;

typedef std::function<void()> CustomDebugFunc;
typedef std::function<void()> NextFrameFunc;

struct Settings
{
	glm::ivec2 mWindowSize;
	std::string mShadersListPath;

	uint32_t mTransformBindingPoint;
	uint32_t mMaterialBindingPoint;

	void Parse(rapidjson::Document& settingsFile);
};

class Engine : public Singleton<Engine>
{
public:
	void Start(const std::string& settingsFilePath);
	void Run();
	void Stop();
	
	template<typename Project>
	void SetScene();

	void AddCustomDebugFunction(CustomDebugFunc DebugFunction);

	void ExecuteOnNextFrameStart(NextFrameFunc NextFrameFunction);
	void ExecuteOnNextFrameEnd(NextFrameFunc NextFrameFunction);

	bool appIsRunning;
	bool stopMoving;
	bool mPause;

	Camera* GetCamera();
	inline Scene* GetScene() { return pScene; }

	inline const Settings& GetSettings() { return mSettings; }
	inline const std::string& GetSettingsFilePath() { return mSettingsFilePath; }

protected:
	void ClearScene();

	Camera* pCamera;
	Scene* pScene;
	std::vector<CustomDebugFunc> CustomDebugFunctions;
	
	std::vector<NextFrameFunc> StartFrameFuncs;
	std::vector<NextFrameFunc> EndFrameFuncs;

	Settings mSettings;
	std::string mSettingsFilePath;
};

template<typename Project>
inline void Engine::SetScene()
{
	ClearScene();
	pScene = new Project();
	pScene->Init();
}
