#include "RayTracing.h"

#include "opengl/ComputeShader.h"
#include "core/ObjectManager.h"
#include "core/Object.h"
#include "core/Components/Material.h"
#include "core/Components/Transform.h"
#include "opengl/ShaderStorageBuffer.h"
#include "utils/Random.h"
#include "core/Engine.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "opengl/Shader.h"
#include "opengl/Renderer.h"

#define MAX_RANDOMS 1000000

void RayTracing::Init()
{
	mLight = new Light(glm::vec3(10.0, 100.0, 40.0));

	Engine::Instance().pCamera->mCameraPos = glm::vec3(-10.0f, 7.0f, 16.0f);
	Engine::Instance().pCamera->pitch = -19.0f;
	Engine::Instance().pCamera->yaw = -56.4f;
	Engine::Instance().pCamera->mSpeed *= 2;
	Engine::Instance().pCamera->CalculateFront();

	DrawShader = new Shader("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	RayOutShader = new Shader("res/shaders/raytraceout.vert", "res/shaders/raytraceout.frag");
		
	TraceRayShader = new ComputeShader("res/shaders/TraceRay.comp");
	//shader->AddShader("res/shaders/Shapes.comp");

	maxPasses = 1;
	ry = 0.2f;
	
	Object* obj = ObjectManager::Instance().AddObject("res/data/ball.json");

	Material* mat = obj->GetComponent<Material>();
		
	shapesStorage = new ShaderStorageBuffer();
	shapesStorage->CreateBuffer(1 * sizeof(RShapes));
	RShapes* shapesData = static_cast<RShapes*>(shapesStorage->GetBufferWritePointer(true));

	for (unsigned int i = 0; i < 1; ++i)
	{
		shapesData[i].object.Kd = mat->mDiffuse;
		shapesData[i].object.Ks = mat->mSpecular;
		shapesData[i].object.alpha = mat->mShininess;
		shapesData[i].object.isActive = true;
		shapesData[i].object.isLight = false;
		shapesData[i].sphereData.center = obj->GetComponent<Transform>()->mPosition;
		shapesData[i].sphereData.radius = obj->GetComponent<Transform>()->mScale.x;
		shapesData[i].shapeType = type_sphere;
	}

	shapesStorage->ReleaseBufferPointer();

	outputColor = new ShaderStorageBuffer();
	int size = Engine::Instance().scrWidth * Engine::Instance().scrHeight;
	outputColor->CreateBuffer(size * sizeof(glm::vec4));
	glm::vec4* colors = static_cast<glm::vec4*>(outputColor->GetBufferWritePointer(true));

	for(int i = 0; i < size; ++i)
	{
		colors[i] = glm::vec4(0.0f,0.0f,0.0f,1.0f);
	}

	outputColor->ReleaseBufferPointer();

	randoms = new ShaderStorageBuffer();
	randoms->CreateBuffer(MAX_RANDOMS * sizeof(float));
	float* randomNums = static_cast<float*>(randoms->GetBufferWritePointer(true));

	auto randomizer = Random::Range(0.0f, 1.0f);
	
	for(int i = 0; i < MAX_RANDOMS; ++i)
	{
		randomNums[i] = randomizer();
	}

	randoms->ReleaseBufferPointer();
}

void RayTracing::Update()
{
	DrawShader->Bind();
	DrawShader->SetUniformMat4f("view", Engine::Instance().pCamera->mView);
	DrawShader->SetUniformMat4f("prevview", Engine::Instance().pCamera->mPrevView);
	DrawShader->SetUniformMat4f("projection", Engine::Instance().pCamera->mProjection);
	DrawShader->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance().pCamera->mView));
	DrawShader->SetUniform3f("lightPos", mLight->position);
	DrawShader->SetUniform3f("light", 3.2f, 3.2f, 3.2f);

	ObjectManager::Instance().UpdateObjects();
	Renderer::Instance().DrawAllObjects(DrawShader);

	shapesStorage->Bind(1);
	outputColor->Bind(2);
	randoms->Bind(3);
	
	TraceRayShader->Bind();
	float rx = ry * Engine::Instance().scrWidth / Engine::Instance().scrHeight;
	glm::vec3 X = rx * Engine::Instance().pCamera->mCameraSide;
	glm::vec3 Y = ry * Engine::Instance().pCamera->mCameraRealUp;
	glm::vec3 Z = -1.0f * Engine::Instance().pCamera->mCameraFront;
	TraceRayShader->SetUniform1f("MAX_FLOAT", std::numeric_limits<float>::max());
	TraceRayShader->SetUniform1i("max_passes", maxPasses);
	TraceRayShader->SetUniform1i("width", Engine::Instance().scrWidth);
	TraceRayShader->SetUniform1i("height", Engine::Instance().scrHeight);
	TraceRayShader->SetUniform1i("Shapes_num", 1);
	TraceRayShader->SetUniform3f("eye", Engine::Instance().pCamera->mCameraPos);
	TraceRayShader->SetUniform3f("X", X);
	TraceRayShader->SetUniform3f("Y", Y);
	TraceRayShader->SetUniform3f("Z", Z);
	
	TraceRayShader->Run(Engine::Instance().scrWidth, Engine::Instance().scrHeight, 1);
	ShaderStorageBuffer::PutMemoryBarrier();
	TraceRayShader->Unbind();

	shapesStorage->Unbind(1);
	outputColor->Unbind(2);
	randoms->Unbind(3);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RayOutShader->Bind();
	unsigned int imageTexture;
	glGenTextures(1, &imageTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, imageTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);


	glm::vec3* outColor = static_cast<glm::vec3*>(outputColor->GetBufferReadPointer());
	
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA32F, Engine::Instance().scrWidth, Engine::Instance().scrHeight, 0, GL_RGBA, GL_FLOAT, &outColor[0]);
	glGenerateMipmap(GL_TEXTURE_2D);
	outputColor->ReleaseBufferPointer();
	int loc = glGetUniformLocation(RayOutShader->GetProgramId(), "imageTex");
	glUniform1i(loc, 1);

	Renderer::Instance().DrawQuad();
	RayOutShader->Unbind();
}

void RayTracing::DebugDisplay()
{
}
