#include "Obstacle.h"

bool Wall::WillCollide(const glm::vec3& P, const glm::vec3& v)
{
	float v_dot_n = glm::dot(v, mWallNormal);
	float t = glm::dot(mWallPoint - P, mWallNormal) / v_dot_n;

	if (v_dot_n != 0.0f && t >= 0.0f && (t * glm::length(v)) < mDistanceThreshold)
	{
		return true;
	}
	else
	{
		return false;
	}
}

glm::vec3 Wall::AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m)
{
	glm::vec3 na = glm::normalize(m);
	glm::vec3 n_cross_na = glm::cross(mWallNormal, na);
	float value = glm::dot(v, n_cross_na);
	float sign = std::signbit(value) ? -1.0f : 1.0f;
	glm::vec3 u = sign * glm::normalize(n_cross_na);

	return u;
}

bool Sphere::WillCollide(const glm::vec3& P, const glm::vec3& v)
{
	return false;
}

glm::vec3 Sphere::AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m)
{
	return glm::vec3(0.0f);
}
