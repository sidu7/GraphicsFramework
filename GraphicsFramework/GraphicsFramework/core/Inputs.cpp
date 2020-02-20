#include "Inputs.h"
#include "Engine.h"
#include "Camera.h"


void Inputs::Init()
{
	SDL_memset(mCurrentKeyState, 0, 512 * sizeof(Uint8));
	SDL_memset(mPreviousKeyState, 0, 512 * sizeof(Uint8));
	SDL_memset(mPrevMouseState, 0, 3 * sizeof(bool));
	SDL_memset(mCurrentMouseState, 0, 3 * sizeof(bool));
}

void Inputs::Update()
{
	SDL_memcpy(mPrevMouseState, mCurrentMouseState, 3 * sizeof(bool));
	SDL_memcpy(mPreviousKeyState, mCurrentKeyState, 512 * sizeof(Uint8));
		
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			Engine::Instance().appIsRunning = false;
		}
		if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			mCurrentMouseState[e.button.button - 1] = true;
		}
		if (e.type == SDL_MOUSEBUTTONUP)
		{
			mCurrentMouseState[e.button.button - 1] = false;
		}
		if (e.type == SDL_MOUSEMOTION)
		{
			Engine::Instance().pCamera->MouseMotionCallBack(e.motion);
		}
	}

	int numberOfFetchedKeys = 0;
	const Uint8* pCurrentKeyStates = SDL_GetKeyboardState(&numberOfFetchedKeys);

	if (numberOfFetchedKeys > 512)
	{
		numberOfFetchedKeys = 512;
	}

	SDL_memcpy(mCurrentKeyState, pCurrentKeyStates, numberOfFetchedKeys * sizeof(Uint8));
}

void Inputs::Close()
{
}

bool Inputs::IsPressed(unsigned int KeyScanCode)
{
	if (KeyScanCode >= 512)
	{
		return false;
	}

	if (mCurrentKeyState[KeyScanCode])
	{
		return true;
	}

	return false;
}

bool Inputs::IsTriggered(unsigned int KeyScanCode)
{
	if (KeyScanCode >= 512)
		return false;

	if (mCurrentKeyState[KeyScanCode] && !mPreviousKeyState[KeyScanCode])
	{
		return true;
	}

	return false;
}

bool Inputs::IsReleased(unsigned int KeyScanCode)
{
	if (KeyScanCode >= 512)
		return false;

	if (!mCurrentKeyState[KeyScanCode] && mPreviousKeyState[KeyScanCode])
	{
		return true;
	}
	return false;
}

bool Inputs::IsMouseClicked(unsigned int KeyScanCode)
{
	if (mCurrentMouseState[KeyScanCode - 1] && !mPrevMouseState[KeyScanCode - 1])
	{
		return true;
	}
	return false;
}

bool Inputs::IsMousePressed(unsigned int KeyScanCode)
{
	if (mCurrentMouseState[KeyScanCode - 1])
	{
		return true;
	}
	return false;
}

bool Inputs::IsMouseReleased(unsigned int KeyScanCode)
{
	if (!mCurrentMouseState[KeyScanCode - 1] && mPrevMouseState[KeyScanCode - 1])
	{
		return true;
	}
	return false;
}

glm::vec2 Inputs::GetMouseLocation()
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	int w, h;
	SDL_GetWindowSize(Engine::Instance().pWindow, &w, &h);
	return glm::vec2(x ,h - y);
}


