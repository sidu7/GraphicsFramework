#include "UniformBufferGL.h"

#include "RHI/OpenGL/DeviceGL.h"
#include "Core/Profiling/Profiler.h"

namespace rhi::gl
{
UniformBufferGL::UniformBufferGL() :
	m_RendererId(0),
	mBinding(0)
{
	GLCall(glGenBuffers(1, &m_RendererId));
}

UniformBufferGL::~UniformBufferGL()
{
	GLCall(glDeleteBuffers(1, &m_RendererId));
}

void UniformBufferGL::Init(uint32_t size, uint32_t binding, const void* data)
{
	mBinding = binding;

	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, size, data? data : NULL, GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UniformBufferGL::AddData(uint32_t size, const void* data, uint32_t offset) const
{
	GF_PROFILE_SCOPE("UniformBufferGL::AddData");
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}
}
