#pragma once
#include "Singleton.h"

#define NO_IMGUI 0

union SDL_Event;

class ImguiManager : public Singleton<ImguiManager>
{
public:
	void Init();
	void StartFrame();
	void Update();
	void Close();
	
	bool IsAnyWindowFocused() const;
	void ProcessWindowEvent(const SDL_Event* windowEvent);
private:
	void Render();
};
