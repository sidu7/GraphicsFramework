#pragma once
#include "glm/glm.hpp"

class Camera
{
public:
	void Init(int FOVangle, int nearPlane, int farPlane);
	void Update();

public:
	float mSpeed;
	glm::mat4 mView;
	glm::mat4 mProjection;

private:
	glm::vec3 mCameraPos;
	glm::vec3 mCameraFront;
	glm::vec3 mCameraUp;
};