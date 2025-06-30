#include "Core/Scene.h"

class VertexBuffer;
class Shader;

class VertexBufferTest : public Scene
{
public:
	virtual ~VertexBufferTest() {}
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void RenderObject(class Object* object, class Shader* shader) override;
	virtual void DebugDisplay() override;

protected:
	Shader* BasicShader;
	VertexBuffer* VBuffer;

	float TestTime;
};