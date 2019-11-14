#pragma once
#include "Singleton.h"
#include "../opengl/Shader.h"
#include "../opengl/Model.h"
#include "../opengl/FrameBuffer.h"
#include "../opengl/ComputeShader.h"
#include "../opengl/UniformBuffer.h"
#include "Light.h"

class Object;

class Scene
{
	SINGLETON(Scene)
public:
	void Init();
	void Draw();

private:
	Shader* shader;
	Shader* lighting;
	Shader* ambient;
	Shader* shadow;
	Shader* locallight;

	//Blur Compute Shader
	ComputeShader* blurHorizontal;
	ComputeShader* blurVertical;
	int blurSize;
	std::vector<float> blurWeights;
	UniformBuffer* block;
	float biasAlpha;

	//Blur NormalShader
	Shader* blurShader;
	FrameBuffer* BlurFBO[2];

	Light* light;
	FrameBuffer* G_Buffer;
	FrameBuffer* ShadowMap;
	int gBuffershow;
	bool showLocalLights;
	float angle;
	glm::vec3 lightColors[40][40];
	glm::vec3 trans1, trans2, trans3;
	Texture* horizontalBlurred;
	Texture* blurredShadowMap;

	// PBL-IBL
	Object* skyDome;
	Shader* skyDomeShader;
	Texture* skyDomeTexture;
};