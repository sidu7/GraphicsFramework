#pragma once

class UniformBuffer
{
private:
	unsigned int m_RendererId;
public:
	UniformBuffer(unsigned int size, const void* data = nullptr);
	~UniformBuffer();

	void Bind(unsigned int bindPoint) const;
	void Unbind() const;
	void SubData(unsigned int size, const void* data) const;
};
