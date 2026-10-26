#pragma once
#include "Rendering/ShaderStorageBuffer.h"

namespace rhi::gl
{
class ShaderStorageBufferGL : public ShaderStorageBuffer
{
public:
	ShaderStorageBufferGL();
	~ShaderStorageBufferGL();
	
	void Bind(unsigned int bindPoint) const;
	void Unbind(unsigned int bindPoint) const;
	void CreateBuffer(unsigned int size);
	void* GetBufferWritePointer(const bool invalidateOldData = false) const;
	void* GetBufferReadPointer() const;
	void* GetBufferReadWritePointer() const;
	void ReleaseBufferPointer() const;
	static void PutMemoryBarrier();

	unsigned int mSize;
	unsigned int mRendererID;
};
}
