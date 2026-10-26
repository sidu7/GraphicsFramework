#include "ShaderStorageBufferVK.h"
#include "Rendering/Renderer.h"


namespace rhi::vk
{
ShaderStorageBufferVK::ShaderStorageBufferVK() : 
	mRendererID(0), 
	mSize(0)
{
}

ShaderStorageBufferVK::~ShaderStorageBufferVK()
{
}

void ShaderStorageBufferVK::Bind(unsigned int bindPoint) const
{
}

void ShaderStorageBufferVK::Unbind(unsigned int bindPoint) const
{
}

void ShaderStorageBufferVK::CreateBuffer(unsigned int size)
{
}

void ShaderStorageBufferVK::ReleaseBufferPointer() const
{
}

void ShaderStorageBufferVK::PutMemoryBarrier()
{
}

void* ShaderStorageBufferVK::GetBufferWritePointer(const bool invalidateOldData) const
{
	return nullptr;
}

void* ShaderStorageBufferVK::GetBufferReadPointer() const
{
	return nullptr;
}

void* ShaderStorageBufferVK::GetBufferReadWritePointer() const
{
	return nullptr;
}
}
