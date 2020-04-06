#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <algorithm>

const int type_sphere = 1;
const int type_box = 2;
const int type_cylinder = 3;
const int type_triangle = 4;

struct Obj
{
	glm::vec3 Kd, Ks, Kt;
	float Ior, alpha;
	bool isLight;
	bool isActive;
};

struct Sphere
{
	glm::vec3 center;
	float radius;
};

struct Slab
{
	glm::vec3 N;
	float d0, d1;
};

struct Box
{
	Slab slabs[3];
	glm::vec3 corner;
	glm::vec3 diagonal;

	void Init(glm::vec3 _c, glm::vec3 _d)
	{
		corner = _c;	diagonal = _d;
		slabs[0].N = glm::vec3(1, 0, 0);
		slabs[0].d0 = -corner.x;
		slabs[0].d1 = -corner.x - diagonal.x;

		slabs[1].N = glm::vec3(0, 1, 0);
		slabs[1].d0 = -corner.y;
		slabs[1].d1 = -corner.y - diagonal.y;

		slabs[2].N = glm::vec3(0, 0, 1);
		slabs[2].d0 = -corner.z;
		slabs[2].d1 = -corner.z - diagonal.z;
	}
};

struct Cylinder
{
	glm::vec3 base;
	glm::vec3 axis;
	float radius;
};

struct Triangle
{
	glm::vec3 v0, v1, v2;
	glm::vec3 n0, n1, n2;
	glm::vec2 t0, t1, t2;
};

struct RShapes
{
	Obj object;
	int shapeType;

	Sphere sphereData;
	Box boxData;
	Cylinder cylinderData;
	Triangle triangleData;
};
