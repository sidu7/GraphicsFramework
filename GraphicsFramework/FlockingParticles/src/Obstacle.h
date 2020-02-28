#pragma once
#include <glm/glm.hpp>

class Obstacle
{
public:
	Obstacle(float d) : mDistanceThreshold(d) {}
	virtual ~Obstacle() {}
	virtual bool WillCollide(const glm::vec3& P, const glm::vec3& v) = 0;
	virtual glm::vec3 AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m) = 0;

	float mDistanceThreshold;
};

class Wall : public Obstacle
{
public:
	Wall(glm::vec3 p, glm::vec3 n, float d, bool i, glm::vec3 s) : Obstacle(d), mWallPoint(p), mWallNormal(n),
	mInfiniteWall(i), mScale(s) {}
	bool WillCollide(const glm::vec3& P, const glm::vec3& v) override;
	glm::vec3 AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m) override;
	
	glm::vec3 mWallPoint;
	glm::vec3 mWallNormal;
	bool mInfiniteWall;
	glm::vec3 mScale;
};

class Sphere : public Obstacle
{
public:
	Sphere(glm::vec3 c, float r, float d): Obstacle(d), mCenter(c), mRadius(r) {}
	bool WillCollide(const glm::vec3& P, const glm::vec3& v) override;
	glm::vec3 AvoidanceDirection(const glm::vec3& P, const glm::vec3& v, const glm::vec3& m) override;

	glm::vec3 mCenter;
	float mRadius;
};