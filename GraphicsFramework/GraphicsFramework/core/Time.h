#pragma once
#include "Singleton.h"

class Time
{
	SINGLETON(Time);
public:
	void Init(unsigned int requiredFrameRate);
	void FrameStart();
	void FrameEnd();
	void SetFrameRate(int frameRate)  { neededTime = 1000 / frameRate; }
private:
	unsigned int startTime;
	unsigned int endTime;
	unsigned int neededTime;
public:
	float deltaTime;
};
