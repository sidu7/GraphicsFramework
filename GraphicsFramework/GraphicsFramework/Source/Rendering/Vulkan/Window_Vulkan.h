#pragma once

#include "Core/Window.h"

class Window_Vulkan : public Window
{
public:

	virtual void Init(int width, int height) override;
	virtual void Close() override;
};