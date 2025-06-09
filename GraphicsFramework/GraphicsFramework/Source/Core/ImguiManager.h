#pragma once
#include "Singleton.h"

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
