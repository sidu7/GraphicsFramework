#pragma once

#include "Core/Window.h"

class Window_OpenGL : public Window
{
public:

	virtual void Init(int width, int height) override;
	virtual void Close() override;

	SDL_GLContext GetContext() const;
public:
	SDL_GLContext glContext;
};