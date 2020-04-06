#include "Camera.h"
#include "Inputs.h"
#include "Time.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Engine.h"
#include "Imgui/imgui.h"
#include <iostream>


void Camera::Init(float FOVangle,float nearPlane, float farPlane)
{
	mSpeed = 20.0f;
	mCameraPos = glm::vec3(0.0f, 10.0f, 20.0f);
	mCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	mCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	mCameraSide = glm::vec3(0.0f);
	mCameraRealUp = glm::vec3(0.0f);
	glm::vec2 wSize = Engine::Instance().GetWindowSize();
	mProjection = glm::perspective(FOVangle, wSize.x/wSize.y, nearPlane, farPlane);
	pitch = 0.0f;
	yaw = -90.0f;
	CalculateFront();
	mView = glm::mat4(1.0f);
}

void Camera::Update()
{
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_W))
	{
		if (Inputs::Instance().IsMousePressed(SDL_BUTTON_RIGHT))
		{
			mCameraPos += mSpeed * Time::Instance().deltaTime * mCameraUp;
		}
		else
		{
			mCameraPos += mSpeed * Time::Instance().deltaTime * mCameraFront;
		}
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_A))
	{
		mCameraPos -= mSpeed * Time::Instance().deltaTime * glm::cross(mCameraFront, mCameraUp);
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_S))
	{
		if (Inputs::Instance().IsMousePressed(SDL_BUTTON_RIGHT))
		{
			mCameraPos -= mSpeed * Time::Instance().deltaTime * mCameraUp;
		}
		else
		{
			mCameraPos -= mSpeed * Time::Instance().deltaTime * mCameraFront;
		}
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_D))
	{
		mCameraPos += mSpeed * Time::Instance().deltaTime * glm::cross(mCameraFront,mCameraUp);
	}

	mPrevView = mView;
	mView = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);

	mCameraSide = glm::cross(mCameraUp, mCameraFront);
	mCameraRealUp = glm::cross(mCameraSide, mCameraFront);
}

void Camera::MouseMotionCallBack(SDL_MouseMotionEvent& mouseEvent)
{
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
	{
		lastMouseX = mouseEvent.x;
		lastMouseY = mouseEvent.y;
		return;
	}

	glm::vec2 wSize = Engine::Instance().GetWindowSize();
	if (Inputs::Instance().IsMousePressed(SDL_BUTTON_LEFT))
	{
		float sensitivity = 0.05f;
		pitch += (lastMouseY - mouseEvent.y) * sensitivity;
		yaw += (mouseEvent.x - lastMouseX) * sensitivity;
		lastMouseY = mouseEvent.y;
		lastMouseX = mouseEvent.x;

		
		if (pitch >  89.0f)
			pitch =  89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		CalculateFront();
	}
	else
	{
		lastMouseX = mouseEvent.x;
		lastMouseY = mouseEvent.y;
	}
}

void Camera::CalculateFront()
{
	glm::vec3 front;
	front.x = glm::cos(glm::radians(pitch)) * glm::cos(glm::radians(yaw));
	front.y = glm::sin(glm::radians(pitch));
	front.z = glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw));
	mCameraFront = glm::normalize(front);
}
