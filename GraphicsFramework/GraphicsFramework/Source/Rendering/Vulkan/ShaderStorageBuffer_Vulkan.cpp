#include "ShaderStorageBuffer_Vulkan.h"
#include "Rendering/Renderer.h"


ShaderStorageBuffer_Vulkan::ShaderStorageBuffer_Vulkan() : 
	mRendererID(0), 
	mSize(0)
{
}

ShaderStorageBuffer_Vulkan::~ShaderStorageBuffer_Vulkan()
{
}

void ShaderStorageBuffer_Vulkan::Bind(unsigned int bindPoint) const
{
}

void ShaderStorageBuffer_Vulkan::Unbind(unsigned int bindPoint) const
{
}

void ShaderStorageBuffer_Vulkan::CreateBuffer(unsigned int size)
{
}

void ShaderStorageBuffer_Vulkan::ReleaseBufferPointer() const
{
}

void ShaderStorageBuffer_Vulkan::PutMemoryBarrier()
{
}

void* ShaderStorageBuffer_Vulkan::GetBufferWritePointer(const bool invalidateOldData) const
{
	return nullptr;
}

void* ShaderStorageBuffer_Vulkan::GetBufferReadPointer() const
{
	return nullptr;
}

void* ShaderStorageBuffer_Vulkan::GetBufferReadWritePointer() const
{
	return nullptr;
}