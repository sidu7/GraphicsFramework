#pragma once
#include "Singleton.h"

class Time : public Singleton<Time>
{
public:
	void Init(unsigned int requiredFrameRate);
	void FrameStart();
	void FrameEnd();
	void SetFrameRate(int frameRate);
private:
	unsigned int startTime;
	unsigned int endTime;
	unsigned int neededTime;
public:
	float deltaTime;
};
