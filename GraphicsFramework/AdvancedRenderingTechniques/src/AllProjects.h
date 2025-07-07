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

struct AOParams
{
	int AONum;
	alignas(4) float AORadius;
	alignas(4) float AOScale;
	alignas(4) float AOContrast;
};

struct LightData
{
	glm::vec3 Ambient;
	alignas(16) glm::vec3 Light;
	alignas(4) float Exposure;
	alignas(4) float Contrast;
	alignas(4) bool ShowIBLDiffuse;
	alignas(4) bool ShowIBLSpecular;
	alignas(4) int32_t ShowGBuffer;
	alignas(4) float BiasAlpha;
};

struct DynamicLightData
{
	glm::mat4 ShadowMatrix;
	alignas(16) glm::vec3 LightPos;
};

struct LocalLightData
{
	glm::mat4 ModelMatrix;
	alignas(16) glm::vec3 LightPos;
	alignas(16) glm::vec3 Light;
	alignas(4) float LightRadius;
};

class AllProjects : public Scene
{
public:
	~AllProjects();
	virtual void Init() override;
	virtual void Close() override;
	virtual void Update() override;
	virtual void DebugDisplay() override;
	virtual void RenderObject(class Object* object, Shader* shader) override;

private:
	void HammersleyRandomPoints();
	void UpdateLightDataUbo();
private:
	Shader* baseShader;
	Shader* lighting;
	Shader* locallight;
	Shader* shadow;

	// Ubos
	UniformBuffer* globalMatrices;
	UniformBuffer* shadowMatrices;
	UniformBuffer* lightDataUbo;
	UniformBuffer* lightDynamicDataUbo;
	UniformBuffer* localLightDataUbo;
	UniformBuffer* AoUbo;

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
	struct
	{
		float N;
		float hammersley[2 * 100];
	} Hblock;
	UniformBuffer* HUniBlock;

	// Ubo Data
	LightData lightData;
	DynamicLightData dynamicLightData;
	LocalLightData localLightData;

	//SSAO
	Shader* AOShader;
	FrameBuffer* BlurredAO;
	ComputeShader* BilateralHorizontal;
	ComputeShader* BilateralVertical;
	Texture* HorizontalBlurredAO;
	Texture* ResultBlurredAO;
	AOParams AoParameters;
};