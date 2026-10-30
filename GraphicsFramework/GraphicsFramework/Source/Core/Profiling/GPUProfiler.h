#pragma once

// GPUProfiler.h — GPU timing via query pools (Vulkan timestamps / OpenGL timer queries).
// Backend is selected at compile time via RENDERER_OPENGL or RENDERER_VULKAN.

#include "Core/Profiling/ProfilerInternal.h"
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
    GF_GPU_QUERY_HANDLE endHandle;
    std::string   objectName;
    std::string   shaderName;
    std::string   materialName;
    uint32_t      instanceID;
    bool          active = false;

    // SDL2 fallback timestamps (used when timer queries are unavailable)
    uint64_t      fallbackBeginTicks = 0;
    uint64_t      fallbackEndTicks = 0;
    bool          fallbackRecorded = false;
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
#ifdef RENDERER_VULKAN
    bool HasCalibratedTimestamps() const { return mCalibratedTimestampsSupported; }
    uint64_t GetGPUClockTimestamp() const { return mGPUClockTimestamp; }
    uint64_t GetGPUClockPrecision() const { return mGPUPrecision; }
#endif

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
    uint32_t                        mNextQueryIndex = 0;   // sequential allocator, reset each frame
    uint32_t                        mActiveCount = 0;
    uint32_t                        mUsedQueryCount = 0;   // completed queries in the frame being recorded
    bool                            mOverflowWarned = false;

    // Frame-level GPU time (from calibrated timestamps)
    double                          mFrameGPUTimeMs = 0.0;

    // Collected draw call samples for this frame
    std::vector<DrawCallSample>     mDrawCallSamples;
    uint32_t                        mNextDrawInstance = 0;

    // Shared: tracks active draw call query index (both backends)
    uint32_t                        mCurrentDrawCallIdx = UINT32_MAX;

    // True until the first frame's GPU work has been submitted and resolved
    bool                            mFirstFrame = true;

    // Index of the fence used by the previous frame (to wait on during ResolveQueries)
    int32_t                         mLastFrameFenceIdx = -1;

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
    VkDevice                        mDevice = VK_NULL_HANDLE;
    VkQueryPool                     mQueryPool = VK_NULL_HANDLE;
    VkCommandBuffer                 mCmdBuffer = VK_NULL_HANDLE;
    VkDevice                        mTimestampDevice = VK_NULL_HANDLE;
    float                           mTimestampPeriod = 1.0f;

    // VK_EXT_calibrated_timestamps support
    PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT mGetCalibrateableTimeDomainsEXT = nullptr;
    PFN_vkGetCalibratedTimestampsEXT                   mGetCalibratedTimestampsEXT = nullptr;
    VkDevice                                            mCalibratedDevice = VK_NULL_HANDLE;
    uint64_t                                            mGPUClockTimestamp = 0;
    uint64_t                                            mGPUPrecision = 0;
    bool                                                mCalibratedTimestampsSupported = false;
#elif defined(RENDERER_OPENGL)
    GLuint           mQueryProgram = 0; // ARB_timer_query doesn't need a program
    bool             mTimerQuerySupported = false;
#endif
};
