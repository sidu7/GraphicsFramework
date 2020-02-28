#pragma once
#include "Components.h"


class Move : public Component
{
	GF_COMPONENT(Move);
public:
	float mTime;
	float mSpeed;
	bool mHorizontal;
	float mCurrentTime;
	bool mFlip;
	float mWait;
	bool mWaiting;
	float mWaitTime;

	void Update() override;

	void Serialize(rapidjson::Value::Object data) override;
};