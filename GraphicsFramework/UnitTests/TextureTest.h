#include "Core/Scene.h"
#include "Core/Core.h"

class VertexBuffer;
class Texture;
class Shader;
class IndexBuffer;
class UniformBuffer;

class TextureTest : public Scene
{
public:
	virtual ~TextureTest() {}
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void RenderObject(class Object* object, class Shader* shader) override;
	virtual void DebugDisplay() override;

protected:
	Shader* BasicShader;
	VertexBuffer* pVBuffer;
	IndexBuffer* pIBuffer;
	UniformBuffer* pUBuffer;
	uint32_t UboBinding = 2;
	Texture* pTexture;
	uint32_t TexBinding = 0;

	float RunTime;
	bool RotateX;
	bool RotateY;
	bool RotateZ;
};