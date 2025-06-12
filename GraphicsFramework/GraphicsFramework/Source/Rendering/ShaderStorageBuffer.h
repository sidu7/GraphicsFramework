#pragma once

class ShaderStorageBuffer
{
public:
	virtual ~ShaderStorageBuffer() {}
	
	virtual void Bind(unsigned int bindPoint) const = 0;
	virtual void Unbind(unsigned int bindPoint) const = 0;
	virtual void CreateBuffer(unsigned int size) = 0;
	virtual void* GetBufferWritePointer(const bool invalidateOldData = false) const = 0;
	virtual void* GetBufferReadPointer() const = 0;
	virtual void* GetBufferReadWritePointer() const = 0;
	virtual void ReleaseBufferPointer() const = 0;
};
