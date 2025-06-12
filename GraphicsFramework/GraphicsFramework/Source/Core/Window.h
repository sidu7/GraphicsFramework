#pragma once

#include "Core.h"

class WindowFactory
{
public:
	static Window* Create();
};

class Window : public SingletonByFactory<Window, WindowFactory>
{
public:
	Window() = default;
	virtual ~Window() = default;
	virtual void Init(int width, int height) = 0;
	virtual void Close() = 0;

	virtual SDL_Window* GetWindow() const;
	virtual SDL_GLContext GetContext() const;
	virtual glm::vec2 GetWindowSize();
	virtual int GetWidth();
	virtual int GetHeight();

protected:
	SDL_Window* pWindow;
	int scrWidth, scrHeight;
	SDL_GLContext glContext;
};