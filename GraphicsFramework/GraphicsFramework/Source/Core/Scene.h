#pragma once

class Scene
{
public:
	Scene() = default;
	virtual ~Scene() = default;
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void RenderObject(class Object* object, class Shader* shader) = 0;
	virtual void DebugDisplay() = 0;
};