#pragma once

// GPUProfiler.h — GPU timing via query pools (Vulkan timestamps / OpenGL timer queries).
// Backend is selected at compile time via RENDERER_OPENGL or RENDERER_VULKAN.

#include "Profiling/ProfilerInternal.h"
#include "Core/Singleton.h"
#include <vector>
#include <string>
#include <cstdint>

// Opaque backend query handle
#ifdef RENDERER_VULKAN
#  include <vulkan/vulkan.h>
#  define GF_GPU_QUERY_HANDLE uint32_t
#elif defined(RENDERER_OPENGL)
#  include <GL/glew.h>
#  define GF_GPU_QUERY_HANDLE GLuint
#else
#  error "Must define RENDERER_OPENGL or RENDERER_VULKAN"
#endif

struct GPUQuery
{
    GF_GPU_QUERY_HANDLE handle;
    std::string   objectName;
    std::string   shaderName;
    std::string   materialName;
    uint32_t      instanceID;
    bool          active = false;
};

class GPUProfiler : public Singleton<GPUProfiler>
{
public:
    void Init(uint32_t maxQueries = 256);
    void Close();

    // Called at frame boundaries
    void StartFrame();
    void EndFrame();

    // Profile a draw call — called from Renderer::Draw() hooks
    void BeginDrawCall(const std::string& objectName, const std::string& shaderName, const std::string& materialName);
    void EndDrawCall();

    // Resolve pending queries and collect draw call samples
    void ResolveQueries();

    // Accessors (called from Profiler/Overlay)
    double GetFrameGPUTimeMs() const { return mFrameGPUTimeMs; }
    const std::vector<DrawCallSample>& GetDrawCallSamples() const { return mDrawCallSamples; }
    uint32_t GetQueryPoolSize() const { return mMaxQueries; }

private:
    friend class Singleton<GPUProfiler>;
    GPUProfiler() = default;
    ~GPUProfiler() = default;

    uint32_t AllocQuery();
    void     FreeQuery(uint32_t index);
    void     FlushQueryPool();

private:
    uint32_t                        mMaxQueries = 256;
    std::vector<GPUQuery>           mQueries;
    uint32_t                        mNextQueryIndex = 0;
    uint32_t                        mActiveCount = 0;

    // Frame-level GPU time (from calibrated timestamps)
    double                          mFrameGPUTimeMs = 0.0;

    // Collected draw call samples for this frame
    std::vector<DrawCallSample>     mDrawCallSamples;
    uint32_t                        mNextDrawInstance = 0;

    // Backend-specific initialization
#ifdef RENDERER_VULKAN
    void InitVulkan();
    void CloseVulkan();
#elif defined(RENDERER_OPENGL)
    void InitOpenGL();
    void CloseOpenGL();
#endif

    // Backend-specific resources (initialized in Init())
#ifdef RENDERER_VULKAN
    VkDevice         mDevice = VK_NULL_HANDLE;
    VkQueryPool      mQueryPool = VK_NULL_HANDLE;
    VkCommandBuffer  mCmdBuffer = VK_NULL_HANDLE;
    VkDevice         mTimestampDevice = VK_NULL_HANDLE;
    float            mTimestampPeriod = 1.0f;
#elif defined(RENDERER_OPENGL)
    GLuint           mQueryProgram = 0; // ARB_timer_query doesn't need a program
    bool             mTimerQuerySupported = false;
#endif
};
