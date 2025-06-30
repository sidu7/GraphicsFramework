#pragma once
#include "Core/Scene.h"

class Shader;
class Light;
class FrameBuffer;
class Texture;
class Object;
class UniformBuffer;

class MotionBlur : public Scene
{
public:
	~MotionBlur();
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void DebugDisplay() override;
	virtual void RenderObject(Object* object, Shader* shader) override;

private:
	Shader* baseShader;
	Shader* lighting;
	Shader* ambient;

	Light* light;
	FrameBuffer* G_Buffer;
	int gBuffershow;
	UniformBuffer* globalMatrices;

	Shader* skyDomeShader;
	Texture* skyDomeTexture;
	float exposure, contrast;
	Object* skyDome;

	bool MotionBlurOn;
	bool PerPixel;
	bool ReconBlur;
	int k;
	int S;
	FrameBuffer* Color;
	FrameBuffer* TileMax;
	Shader* TileMaxShader;
	FrameBuffer* NeighbourMax;
	Shader* NeighbourMaxShader;

	Shader* ReconstructionShader;
	Shader* MotionBlurShader;
};
