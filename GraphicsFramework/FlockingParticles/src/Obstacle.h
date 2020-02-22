#pragma once
#include <glm/glm.hpp>

class Obstacle
{
public:
	virtual ~Obstacle() {}
	virtual bool WillCollide(const glm::vec3& P, const glm::vec3& v) = 0;
	virtual glm::vec3 AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m) = 0;
};

class Wall : public Obstacle
{
public:
	Wall(glm::vec3 p, glm::vec3 n, float d) : mWallPoint(p), mWallNormal(n), mDistanceThreshold(d) {}
	bool WillCollide(const glm::vec3& P, const glm::vec3& v) override;
	glm::vec3 AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m) override;
	
	glm::vec3 mWallPoint;
	glm::vec3 mWallNormal;
	float mDistanceThreshold;
};

class Sphere : public Obstacle
{
public:
	bool WillCollide(const glm::vec3& P, const glm::vec3& v) override;
	glm::vec3 AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m) override;
};