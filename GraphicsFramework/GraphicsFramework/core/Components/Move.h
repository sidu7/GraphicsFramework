#pragma once
#include "Components.h"


class Move : public Component
{
public:
	float mTime;
	float mSpeed;
	bool mHorizontal;
	float mCurrentTime;
	bool mFlip;
	float mWait;
	bool mWaiting;
	float mWaitTime;

	void Update(Shader* shader) override;

	void Serialize(rapidjson::Value::Object data) override;
};