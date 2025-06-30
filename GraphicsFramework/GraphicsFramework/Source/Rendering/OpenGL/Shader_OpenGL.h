/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Shader.h"

class Shader_OpenGL : public Shader
{
public:
	Shader_OpenGL();
	virtual ~Shader_OpenGL();

	virtual void Init(std::string vertexShaderId, std::string fragmentShaderId) override;
	
	virtual void Uses(const FrameBuffer* framebuffer) override;
	virtual void Uses(const Texture* texture, unsigned int slot = 0) override;
	virtual void Uses(const UniformBuffer* uniformBuffer, unsigned int binding = 0) override;
	virtual void Uses(const VertexBuffer* vertexBuffer) override;
	virtual void Uses(const IndexBuffer* indexBuffer) override;

	unsigned int m_RendererID;

protected:
	void SetUniform1i(const std::string& name, int i);
	void SetUniform1f(const std::string& name, float v);
	void SetUniform2f(const std::string& name, float v1, float v2);
	void SetUniform2f(const std::string& name, glm::vec2 v);
	void SetUniform3f(const std::string& name, float v1, float v2, float v3);
	void SetUniform3f(const std::string& name, glm::vec3 v);
	void SetUniform4f(const std::string& name, float v1, float v2, float v3, float v4);
	void SetUniform4f(const std::string& name, glm::vec4 v);
	void SetUniformMat4f(const std::string& name, glm::mat4 matrix);
	void SetUniformMat4fLoc(int location, glm::mat4 matrix);
	int GetUniformLocation(const std::string& name);
	void SetUniform2fvLoc(int location, glm::vec2 vector);
	void SetUniform3fvLoc(int location, glm::vec3 vector);
	void SetUniform4fvLoc(int location, glm::vec4 vector);
	void SetUniformBlock(const std::string& name, unsigned int bindPoint);

	unsigned int CreateProgram(const ShaderSource& shaderSource);
	unsigned int CompileShader(unsigned int type, const std::vector<char>& Source);
};