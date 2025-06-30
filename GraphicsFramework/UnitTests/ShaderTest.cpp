#include "ShaderTest.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/ShaderManager.h"

void ShaderTest::Init()
{
	BasicShader = RenderingFactory::Instance()->CreateShader();
	BasicShader->Init("SHADER_TEST");
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