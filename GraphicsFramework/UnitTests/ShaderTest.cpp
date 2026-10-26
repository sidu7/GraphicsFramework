#include "ShaderTest.h"

#include "Core/Core.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "UnitTestShaderID.h"

void ShaderTest::Init()
{
	BasicShader = Renderer::Instance()->CreateShader();
	BasicShader->Init(static_cast<int32_t>(UnitTestShaderID::SHADER_TEST));
}

void ShaderTest::Close()
{
	delete BasicShader;
}

void ShaderTest::Update()
{
	Renderer::Instance()->Draw(BasicShader, nullptr, 3, 0);
}

void ShaderTest::RenderObject(class Object* object, class Shader* shader)
{
}

void ShaderTest::DebugDisplay()
{
}
