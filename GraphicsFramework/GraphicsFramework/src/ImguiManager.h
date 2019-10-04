#pragma once
#include "Singleton.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_opengl3.h"
#include "Imgui/imgui_impl_sdl.h"

class ImguiManager
{
	SINGLETON(ImguiManager)
public:
	void Init();
	void StartFrame();
	void Update();
	void Close();
	
private:
	void Render();
};