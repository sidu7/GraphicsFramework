#pragma once
#include <string>

class ComputeShader
{
private:
	unsigned int m_RendererID;
	std::string m_ShaderFilePath;
public:
	ComputeShader(std::string path);
	~ComputeShader();

	inline unsigned int GetProgramId() { return m_RendererID; }
private:
	unsigned int CreateProgram(std::string& shaderSource);
	unsigned int CompileShader(unsigned int type, std::string& Source);
	std::string ParseShader(std::string shaderFilePath);
public:
	void Bind() const;
	void Unbind() const;
	void Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const;

	void SetInputUniformImage(std::string name, unsigned int textureId, unsigned int imageUnit, unsigned int channels) const;
	void SetOutputUniformImage(std::string name, unsigned int textureId, unsigned int imageUnit, unsigned int channels) const;
	void SetUniformBlock(std::string name, unsigned int bindPoint);
	void SetUniform1i(const std::string& name, int i);
	int GetUniformLocation(const std::string& name);
};