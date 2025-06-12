#include <Core/Scene.h>
#include <vector>
#include <glm/glm.hpp>

#define HBlockSize 30

class Shader;
class ComputeShader;
class UniformBuffer;
class FrameBuffer;
class Light;
class Object;
class Texture;

class AllProjects : public Scene
{
public:
	~AllProjects();
	virtual void Init() override;
	virtual void Update() override;
	virtual void DebugDisplay() override;
	virtual void RenderObject(class Object* object, Shader* shader) override;

private:
	void HammersleyRandomPoints();

private:
	Shader* baseShader;
	Shader* lighting;
	Shader* locallight;
	Shader* shadow;
	UniformBuffer* globalMatrices;

	// AO
	Shader* ambient;
	Shader* ambientNoAO;
	bool showAO;

	//Blur Compute Shader
	ComputeShader* blurHorizontal;
	ComputeShader* blurVertical;
	int blurSize;
	std::vector<float> blurWeights;
	UniformBuffer* block;
	float biasAlpha;

	//Blur NormalShader
	Shader* blurShader;
	bool softShadows;

	Light* light;
	bool lighton;
	
	/*
	 * 0 - Normals
	 * 1 - WorldPos
	 * 2 - Diffuse
	 * 3 - Specular,Alpha
	 */
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
	Texture* skyDomeIrradiance;
	float exposure;
	float contrast;
	struct
	{
		float N;
		float hammersley[2 * 100];
	} Hblock;
	UniformBuffer* HUniBlock;
	bool IBLDiffuse, IBLSpecular;

	//SSAO
	Shader* AOShader;
	FrameBuffer* BlurredAO;
	ComputeShader* BilateralHorizontal;
	ComputeShader* BilateralVertical;
	Texture* HorizontalBlurredAO;
	Texture* ResultBlurredAO;
	int AONum;
	float AORadius;
	float AOScale;
	float AOContrast;
};