#include "Obstacle.h"
#include <glm/gtx/norm.inl>

bool Wall::WillCollide(const glm::vec3& P, const glm::vec3& v)
{
	float v_dot_n = glm::dot(v, mWallNormal);
	glm::vec3 vec = P - mWallPoint;
	float t = glm::dot(vec, mWallNormal);

	float hyp = glm::length2(vec);
	float height = sqrt(hyp - t * t);

	glm::vec3 hvec = glm::normalize(vec);

	/*bool result = false;

	if (!mInfiniteWall)
	{
		if ((mScale.x == 0.0f && mScale.y*4.0f > fabs(hvec.y * height) && mScale.z*4.0f > fabs(hvec.z * height)) ||
			(mScale.y == 0.0f && mScale.x*4.0f > fabs(hvec.x * height) && mScale.z*4.0f > fabs(hvec.z * height)) ||
			(mScale.z == 0.0f && mScale.x*4.0f > fabs(hvec.x * height) && mScale.y*4.0f > fabs(hvec.y * height)))
		{
			result = true;
		}
	}
	else
	{
		result = true;
	}*/
	
	if (v_dot_n != 0.0f && t >= 0.0f && t < mDistanceThreshold)
	{
		return true;
	}

	return false;
}

glm::vec3 Wall::AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m)
{
	/*glm::vec3 na = glm::normalize(m);
	glm::vec3 n_cross_na = glm::cross(mWallNormal, na);
	float value = glm::dot(-v, n_cross_na);
	float sign = std::signbit(value) ? -1.0f : 1.0f;
	glm::vec3 u = glm::normalize(n_cross_na);*/

	glm::vec3 va = glm::normalize(v);
	glm::vec3 u = glm::normalize(mWallNormal + va);
	if(isnan(glm::length(u)))
	{
		__debugbreak();
	}
	return u;
}

bool Sphere::WillCollide(const glm::vec3& P, const glm::vec3& v)
{
	float a = glm::length2(v);
	glm::vec3 vec = P - mCenter;
	float b = glm::dot(2.0f * v, vec);
	float c = glm::length2(vec) - mRadius * mRadius;
	float delta = b * b - 4 * a * c;
	if(delta < 0)
	{
		return false;
	}
	float t = b + sqrt(delta);
	float t_some = -1 / (2 * a) * t * glm::length(v);
	if(t <= 0.0f && t_some < mDistanceThreshold)
	{
		return true;
	}
	return false;
}

glm::vec3 Sphere::AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m)
{
	glm::vec3 vec = mCenter - P;
	float dot_n = glm::dot(m, vec);
	float s = sqrt(mRadius * mRadius - dot_n * dot_n);
	glm::vec3 C_prime = mCenter - dot_n * m;
	glm::vec3 vec_prime = C_prime - P;
	float sign = signbit(glm::dot(glm::cross(vec_prime,v),m)) ? -1.0f : 1.0f;
	float sinT = s / glm::length(vec_prime);
	float cosT = sqrt(1.0f - sinT * sinT);
	glm::vec3 u = cosT * glm::normalize(vec_prime) + sign * sinT * glm::cross(m, vec_prime) / glm::length(vec_prime);
	return u;
}
