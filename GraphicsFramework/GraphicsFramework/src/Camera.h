#pragma once
#include "glm/glm.hpp"
#include "SDL.h"

class Camera
{
public:
	void Init(float FOVangle, float nearPlane, float farPlane);
	void Update();
	void MouseMotionCallBack(SDL_MouseMotionEvent& mouseEvent);
	void CalculateFront();

public:
	float mSpeed;
	glm::mat4 mView;
	glm::mat4 mProjection;
	glm::vec3 mCameraPos;
	float pitch;

private:
	glm::vec3 mCameraFront;
	glm::vec3 mCameraUp;
	float yaw;

	float lastMouseX;
	float lastMouseY;
};