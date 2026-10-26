#pragma once

#include "Core/Window.h"

namespace rhi::vk
{
class WindowVK : public Window
{
public:

	virtual void Init(int width, int height) override;
	virtual void Close() override;
};
}
