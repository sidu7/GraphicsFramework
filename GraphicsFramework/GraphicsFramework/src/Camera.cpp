#include "Camera.h"
#include "Inputs.h"
#include "Time.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Engine.h"

extern Engine* engine;

void Camera::Init(int FOVangle,int nearPlane, int farPlane)
{
	mSpeed = 0.0f;
	mCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	mCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	mCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec2 wSize = engine->GetWindowSize();
	mProjection = glm::perspectiveFov(FOVangle, (int)wSize.x, (int)wSize.y, nearPlane, farPlane);
}

void Camera::Update()
{
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_W))
	{
		mCameraPos += mSpeed * Time::Instance().deltaTime * mCameraFront;
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_A))
	{
		mCameraPos -= mSpeed * Time::Instance().deltaTime * glm::cross(mCameraFront, mCameraUp);
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_S))
	{
		mCameraPos -= mSpeed * Time::Instance().deltaTime * mCameraFront;
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_D))
	{
		mCameraPos += mSpeed * Time::Instance().deltaTime * glm::cross(mCameraFront,mCameraUp);
	}

	mView = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);
}
