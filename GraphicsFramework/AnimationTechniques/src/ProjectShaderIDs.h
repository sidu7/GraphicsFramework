#pragma once
#include <cstdint>

// AnimationTechniques project-local shader IDs — shared across all scenes
enum class AnimationShaderID : int32_t
{
    NONE = 0,
    MODEL_DRAW,
    DRAWING,
    COUNT
};
