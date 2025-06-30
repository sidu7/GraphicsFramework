/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/


#pragma once

#include "Core/Core.h"

enum class VertexFormat
{
	Bool,
	UInt,
	Int,
	Float,
	Vec2,
	Vec3,
	Vec4,
	iVec2,
	iVec3,
	iVec4,
	Mat2,
	Mat3,
	Mat4
};

struct VertexData
{
	VertexFormat mFormat;
	uint32_t mOffset;
};

class VertexBuffer
{
public:
	VertexBuffer() : mStride(0) {}
	virtual ~VertexBuffer() {}

	virtual void SetDataLayout(const std::vector<VertexFormat>& vertexFormats) = 0;
	virtual void Delete() const = 0;

	virtual void AddData(const void* data, unsigned int size) = 0;
	virtual void AddSubData(const void* data, unsigned int size, unsigned offset = 0) = 0;
	virtual void AddDynamicData(const void* data, unsigned int size) = 0;

	uint16_t mStride;
	std::vector<VertexData> mVertexLayouts;

	static inline size_t GetFormatSize(VertexFormat format)
	{
		switch (format)
		{
			case VertexFormat::Bool:
				return sizeof(bool);
				break;
			case VertexFormat::UInt:
				return sizeof(unsigned int);
				break;
			case VertexFormat::Int:
				return sizeof(int);
				break;
			case VertexFormat::iVec2:
				return sizeof(int) * 2;
				break;
			case VertexFormat::iVec3:
				return sizeof(int) * 3;
				break;
			case VertexFormat::iVec4:
				return sizeof(int) * 4;
				break;
			case VertexFormat::Float:
				return sizeof(float);
				break;
			case VertexFormat::Vec2:
				return sizeof(float) * 2;
				break;
			case VertexFormat::Vec3:
				return sizeof(float) * 3;
				break;
			case VertexFormat::Vec4:
				return sizeof(float) * 4;
				break;
			case VertexFormat::Mat2:
				return sizeof(float) * 4;
				break;
			case VertexFormat::Mat3:
				return sizeof(float) * 9;
				break;
			case VertexFormat::Mat4:
				return sizeof(float) * 16;
				break;
			default:
				return 0;
				break;
		}
	}
};
