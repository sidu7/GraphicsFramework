#include "Core/Scene.h"

class ShaderTest : public Scene
{
public:
	virtual ~ShaderTest() {}
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void RenderObject(class Object* object, class Shader* shader) override;
	virtual void DebugDisplay() override;

protected:
	Shader* BasicShader;
};