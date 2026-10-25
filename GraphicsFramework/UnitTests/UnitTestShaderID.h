#pragma once
#include <cstdint>

// UnitTests application-local shader IDs
enum class UnitTestShaderID : int32_t
{
    NONE = 0,
    SHADER_TEST,
    VERTEXBUFFER_TEST,
    INDEXBUFFER_TEST,
    UNIFORMBUFFER_TEST,
    TEXTURE_TEST,
    COUNT
};
