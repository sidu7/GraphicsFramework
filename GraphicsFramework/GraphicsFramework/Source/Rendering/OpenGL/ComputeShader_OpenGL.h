#pragma once

#include "Rendering/ComputeShader.h"

class ComputeShader_OpenGL : public ComputeShader
{
public:
	ComputeShader_OpenGL();
	~ComputeShader_OpenGL();

	virtual void SetShader(std::string path) override;
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

	unsigned int m_RendererID;
	std::string m_ShaderFilePath;	
protected:
	unsigned int CompileShader(unsigned int type, const std::vector<char>& Source);
};
