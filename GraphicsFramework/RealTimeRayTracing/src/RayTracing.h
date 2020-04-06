#pragma once
#include "core/Scene.h"
#include <vector>
#include "Shapes.h"

class Light;
class ComputeShader;
class ShaderStorageBuffer;
class Shader;

class RayTracing : public Scene
{
public:
	~RayTracing() override {}
	void Init() override;
	void Update() override;
	void DebugDisplay() override;

private:
	ComputeShader* TraceRayShader;
	ShaderStorageBuffer* shapesStorage;
	ShaderStorageBuffer* outputColor;
	ShaderStorageBuffer* randoms;
	Shader* DrawShader;
	Shader* RayOutShader;
	Light* mLight;

	float ry;
	int maxPasses;
	std::vector<RShapes> allShapes;
};