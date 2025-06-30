#include "Core/Scene.h"

class VertexBuffer;
class IndexBuffer;
class Shader;

class IndexBufferTest : public Scene
{
public:
	virtual ~IndexBufferTest() {}
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void RenderObject(class Object* object, class Shader* shader) override;
	virtual void DebugDisplay() override;

protected:
	Shader* BasicShader;
	VertexBuffer* VBuffer;
	IndexBuffer* IBuffer;
};