/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include <string>
#include <vector>

#include "Rendering/ComputeShader.h"
#include "Rendering/ShaderRegistry.h"

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class ComputeShaderVK : public ComputeShader
{
public:
	ComputeShaderVK();
	virtual ~ComputeShaderVK();

	virtual void Init(int32_t shaderId) override;
	virtual void Init(const std::string& filePath) override;
	virtual void Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const override;

	virtual void SetInputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const override;
	virtual void SetOutputUniformImage(std::string name, const Texture* texture, unsigned int imageUnit, unsigned int channels) const override;
	virtual void SetUniformBlock(std::string name, unsigned int bindPoint) override;
	virtual void SetUniform1i(const std::string& name, int i) override;
	virtual void SetUniform1f(const std::string& name, float v) override;
	virtual void SetUniform2f(const std::string& name, float v1, float v2) override;
	virtual void SetUniform2f(const std::string& name, glm::vec2 v) override;
	virtual void SetUniform3f(const std::string& name, float v1, float v2, float v3) override;
	virtual void SetUniform3f(const std::string& name, glm::vec3 v) override;
	virtual int GetUniformLocation(const std::string& name) override;

	VkPipeline mPipeline;
	VkPipelineLayout mPipelineLayout;

protected:
	void LoadShaderModule(const std::vector<char>& shaderSource, VkShaderModule& outModule);
	std::string mShaderId;
};
}
