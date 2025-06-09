#pragma once
#include "Core/Scene.h"

class Shader;
class Light;
class FrameBuffer;
class Texture;
class Object;

class MotionBlur : public Scene
{
public:
	~MotionBlur();
	void Init() override;
	void Update() override;
	void DebugDisplay() override;

private:
	Shader* shader;
	Shader* lighting;
	Shader* ambient;

	Light* light;
	FrameBuffer* G_Buffer;
	int gBuffershow;

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
