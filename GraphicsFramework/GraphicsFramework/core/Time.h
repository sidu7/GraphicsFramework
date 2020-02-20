#pragma once
#include "Singleton.h"

class Time
{
	SINGLETON(Time);
public:
	void Init(unsigned int requiredFrameRate);
	void FrameStart();
	void FrameEnd();
private:
	unsigned int startTime;
	unsigned int endTime;
	unsigned int neededTime;
public:
	float deltaTime;
};
