/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Shader.h"
#include "Rendering/ShaderRegistry.h"

namespace rhi::gl
{
class ShaderGL : public Shader
{
public:
	ShaderGL();
	virtual ~ShaderGL();

	virtual void Init(int32_t shaderId) override;
	virtual void Init(const std::string& vertexPath, const std::string& fragmentPath) override;

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

	unsigned int CompileShader(unsigned int type, const std::vector<char>& Source);
};
}
