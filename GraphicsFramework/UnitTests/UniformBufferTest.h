#include "Core/Scene.h"
#include <stdint.h>

class VertexBuffer;
class IndexBuffer;
class Shader;
class UniformBuffer;

class UniformBufferTest : public Scene
{
public:
	virtual ~UniformBufferTest() {}
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void RenderObject(class Object* object, class Shader* shader) override;
	virtual void DebugDisplay() override;

protected:
	Shader* BasicShader;
	VertexBuffer* VBuffer;
	IndexBuffer* IBuffer;
	UniformBuffer* UBuffer;
	uint32_t UboBinding = 2;

	float RunTime;
	bool RotateX;
	bool RotateY;
	bool RotateZ;
};