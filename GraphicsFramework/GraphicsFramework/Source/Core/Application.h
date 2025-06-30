#pragma once

#include "Core/Engine.h"

class Application
{
public:
	virtual std::string GetSettingsFilePath() = 0;

	virtual void Init() = 0;
	virtual void Close() = 0;
};