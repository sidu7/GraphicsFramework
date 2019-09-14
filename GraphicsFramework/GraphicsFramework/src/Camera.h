#pragma once
#include "glm/glm.hpp"
#include "SDL.h"

class Camera
{
public:
	void Init(float FOVangle, float nearPlane, float farPlane);
	void Update();
	void MouseMotionCallBack(SDL_MouseMotionEvent& mouseEvent);

public:
	float mSpeed;
	glm::mat4 mView;
	glm::mat4 mProjection;

private:
	glm::vec3 mCameraPos;
	glm::vec3 mCameraFront;
	glm::vec3 mCameraUp;
	float pitch;
	float yaw;

	float lastMouseX;
	float lastMouseY;
};