#pragma once
#include <string>
#include <glm/glm.hpp>

class ComputeShader
{
private:
	unsigned int m_RendererID;
	std::string m_ShaderFilePath;
public:
	ComputeShader(std::string path);
	~ComputeShader();

	inline unsigned int GetProgramId() { return m_RendererID; }
	void AddShader(std::string path);
	void Bind() const;
	void Unbind() const;
	void Run(unsigned int groupX, unsigned int groupY, unsigned int groupZ) const;
	 
	void SetInputUniformImage(std::string name, unsigned int textureId, unsigned int imageUnit, unsigned int channels) const;
	void SetOutputUniformImage(std::string name, unsigned int textureId, unsigned int imageUnit, unsigned int channels) const;
	void SetUniformBlock(std::string name, unsigned int bindPoint);
	void SetUniform1i(const std::string& name, int i);
	void SetUniform1f(const std::string& name, float v);
	void SetUniform2f(const std::string& name, float v1, float v2);
	void SetUniform2f(const std::string& name, glm::vec2 v);
	void SetUniform3f(const std::string& name, float v1, float v2, float v3);
	void SetUniform3f(const std::string& name, glm::vec3 v);
	int GetUniformLocation(const std::string& name);
	
private:
	unsigned int CompileShader(unsigned int type, std::string& Source);
	std::string ParseShader(std::string shaderFilePath);
};
