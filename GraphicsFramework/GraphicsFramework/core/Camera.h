#pragma once
#include "glm/glm.hpp"
#include "SDL/include/SDL.h"

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
	glm::mat4 mPrevView;
	glm::mat4 mProjection;
	glm::vec3 mCameraPos;
	float pitch;
	float yaw;

	glm::vec3 mCameraFront;
	glm::vec3 mCameraUp;
	glm::vec3 mCameraSide;
	glm::vec3 mCameraRealUp;
private:

	float lastMouseX;
	float lastMouseY;
};