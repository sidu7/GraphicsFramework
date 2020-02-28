#pragma once
#include <glm/mat4x2.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

class MathHelper
{
public:
	static glm::mat4 RotationFromDirection(const glm::vec3& direction)
	{
		float angle = std::atan2(direction.y, direction.x);
		glm::mat4 glmrotXY = glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
		// Find the angle with the xy with plane (0, 0, 1); the - there is because we want to 
		// 'compensate' for that angle (a 'counter-angle')
		float angleZ = -std::asin(direction.z);
		// Make the matrix for that, assuming that Y is your 'side' vector; makes the model 'pitch'
		glm::mat4 glmrotZ = glm::rotate(angleZ, glm::vec3(0.0f, 1.0f, 0.0f));
		return glmrotXY * glmrotZ;
	}
};
