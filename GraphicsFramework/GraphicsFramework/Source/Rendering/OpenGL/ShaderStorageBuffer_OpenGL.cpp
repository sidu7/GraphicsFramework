#include "ShaderStorageBuffer_OpenGL.h"
#include "Rendering/OpenGL/Renderer_OpenGL.h"


ShaderStorageBuffer_OpenGL::ShaderStorageBuffer_OpenGL() : 
	mRendererID(0), 
	mSize(0)
{
	GLCall(glGenBuffers(1, &mRendererID));
}

ShaderStorageBuffer_OpenGL::~ShaderStorageBuffer_OpenGL()
{
	GLCall(glDeleteBuffers(1, &mRendererID));
}

void ShaderStorageBuffer_OpenGL::Bind(unsigned int bindPoint) const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, mRendererID));
}

void ShaderStorageBuffer_OpenGL::Unbind(unsigned int bindPoint) const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, 0));
}

void ShaderStorageBuffer_OpenGL::CreateBuffer(unsigned int size)
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	mSize = size;
	GLCall(glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT));
	//GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_STATIC_DRAW));
}

void ShaderStorageBuffer_OpenGL::ReleaseBufferPointer() const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	GLCall(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));
}

void ShaderStorageBuffer_OpenGL::PutMemoryBarrier()
{
	GLCall(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
}

void* ShaderStorageBuffer_OpenGL::GetBufferWritePointer(const bool invalidateOldData) const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	GLbitfield flag = GL_MAP_WRITE_BIT;
	if (invalidateOldData) flag |= GL_MAP_INVALIDATE_BUFFER_BIT;
	return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mSize, flag);
}

void* ShaderStorageBuffer_OpenGL::GetBufferReadPointer() const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mSize, GL_MAP_READ_BIT);
}

void* ShaderStorageBuffer_OpenGL::GetBufferReadWritePointer() const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
	return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mSize, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
}