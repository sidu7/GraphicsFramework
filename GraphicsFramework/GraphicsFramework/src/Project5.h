#pragma once
#include "Singleton.h"

class Shader;
class Light;
class FrameBuffer;
class Texture;
class Object;

class Project5
{
	SINGLETON(Project5);
public:
	void Init();
	void Draw();

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

	bool MotionBlur;
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