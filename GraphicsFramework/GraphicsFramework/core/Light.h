#pragma once
#include "glm/glm.hpp"

class Light
{
public:
	Light(glm::vec3 _p) : position(_p) {}
	void HandleInputs();
	
	glm::vec3 position;
};
