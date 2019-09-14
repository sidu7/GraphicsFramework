#pragma once
#include <SDL.h>
#include "Singleton.h"
#include <glm/glm.hpp>

class Inputs
{
	SINGLETON(Inputs)
public:	
	void Init();
	void Update();
	void Close();

	bool IsPressed(unsigned int KeyScanCode);
	bool IsTriggered(unsigned int KeyScanCode);
	bool IsReleased(unsigned int KeyScanCode);

	bool IsMouseClicked(unsigned int KeyScanCode);
	bool IsMousePressed(unsigned int KeyScanCode);
	bool IsMouseReleased(unsigned int KeyScanCode);
	glm::vec2 GetMouseLocation();

private:
	void MouseMotionCallBack(SDL_MouseMotionEvent& mouseEvent);

private:
	Uint8 mCurrentKeyState[512];
	Uint8 mPreviousKeyState[512];
	bool mPrevMouseState[3];
	bool mCurrentMouseState[3];
};

