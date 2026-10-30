// GPUProfiler.cpp — GPU profiling via query pools.
// Backend dispatch: RENDERER_VULKAN → timestamp queries, RENDERER_OPENGL → GL_TIME_ELAPSED.
//
// Design: every frame the profiler records a timestamp pair (begin/end) around
// each draw call into a query pool. The pool is reset at the start of the next
// frame and the previous frame's pairs are read back once the GPU has finished
// them. Queries are allocated sequentially (0..N-1) so the results of a frame
// are always a contiguous range.

#include "Core/Profiling/GPUProfiler.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#ifdef RENDERER_VULKAN
#  include "RHI/Vulkan/DeviceVK.h"
#  include "RHI/Vulkan/Internal/CommandBufferVK.h"
#elif defined(RENDERER_OPENGL)
#  include <SDL.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Common interface
// ─────────────────────────────────────────────────────────────────────────────
void GPUProfiler::Init(uint32_t maxQueries)
{
    if (maxQueries == 0)
    {
        maxQueries = 1;
    }

    mMaxQueries = maxQueries;
    mQueries.assign(maxQueries, GPUQuery{});

#ifdef RENDERER_VULKAN
    InitVulkan();
#elif defined(RENDERER_OPENGL)
    InitOpenGL();
#endif
}

void GPUProfiler::Close()
{
#ifdef RENDERER_VULKAN
    CloseVulkan();
#elif defined(RENDERER_OPENGL)
    CloseOpenGL();
#endif

    mQueries.clear();
    mNextQueryIndex = 0;
    mActiveCount = 0;
    mUsedQueryCount = 0;
}

void GPUProfiler::StartFrame()
{
    // Drop the samples copied into the last snapshot; ResolveQueries() refills
    // them with the previous frame's results.
    mDrawCallSamples.clear();
    mFrameGPUTimeMs = 0.0;

    // Resolve the queries recorded by the previous frame. They are read before
    // the pool is reset, and before this frame records anything.
    ResolveQueries();

    // Prepare the pool for the frame we are about to record.
    mNextQueryIndex = 0;
    mActiveCount = 0;
    mUsedQueryCount = 0;
    mNextDrawInstance = 0;
    mCurrentDrawCallIdx = UINT32_MAX;
    mOverflowWarned = false;

#ifdef RENDERER_VULKAN
    if (mQueryPool != VK_NULL_HANDLE)
    {
        vkResetQueryPool(mDevice, mQueryPool, 0, mMaxQueries * 2);
    }
#endif
}

void GPUProfiler::EndFrame()
{
    // Query resolution happens at StartFrame() of the next frame, after the GPU
    // has had time to finish the work recorded this frame.
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw call profiling
// ─────────────────────────────────────────────────────────────────────────────
void GPUProfiler::BeginDrawCall(const std::string& objectName, const std::string& shaderName, const std::string& materialName)
{
    mCurrentDrawCallIdx = UINT32_MAX;

    const uint32_t idx = AllocQuery();
    if (idx == UINT32_MAX)
    {
        return;
    }

    GPUQuery& query = mQueries[idx];
    query.objectName = objectName;
    query.shaderName = shaderName;
    query.materialName = materialName;
    query.instanceID = mNextDrawInstance++;

#ifdef RENDERER_VULKAN
    rhi::vk::Device* vkDevice = rhi::vk::Device::Get();
    if (vkDevice && mQueryPool != VK_NULL_HANDLE)
    {
        const rhi::vk::CommandBufferVK* cmdBuffer = vkDevice->GetCommandBuffer();
        if (cmdBuffer && cmdBuffer->mCommandBuffer != VK_NULL_HANDLE)
        {
            // Timestamp queries use vkCmdWriteTimestamp (vkCmdBeginQuery is not
            // valid for VK_QUERY_TYPE_TIMESTAMP). Each draw owns two slots:
            // even = begin, odd = end.
            query.handle = idx * 2;
            query.endHandle = idx * 2 + 1;

            vkCmdWriteTimestamp(cmdBuffer->mCommandBuffer,
                                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                mQueryPool, query.handle);
            mCurrentDrawCallIdx = idx;
        }
    }
#elif defined(RENDERER_OPENGL)
    if (mTimerQuerySupported)
    {
        glBeginQuery(GL_TIME_ELAPSED, query.handle);
        mCurrentDrawCallIdx = idx;
    }
    else
    {
        // SDL2 performance counter fallback (timer queries unavailable).
        query.fallbackBeginTicks = SDL_GetPerformanceCounter();
        mCurrentDrawCallIdx = idx;
    }
#endif
}

void GPUProfiler::EndDrawCall()
{
    if (mCurrentDrawCallIdx == UINT32_MAX)
    {
        return;
    }

    const uint32_t idx = mCurrentDrawCallIdx;
    mCurrentDrawCallIdx = UINT32_MAX;

#ifdef RENDERER_VULKAN
    rhi::vk::Device* vkDevice = rhi::vk::Device::Get();
    if (vkDevice && mQueryPool != VK_NULL_HANDLE)
    {
        const rhi::vk::CommandBufferVK* cmdBuffer = vkDevice->GetCommandBuffer();
        if (cmdBuffer && cmdBuffer->mCommandBuffer != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(cmdBuffer->mCommandBuffer,
                                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                mQueryPool, mQueries[idx].endHandle);
            mUsedQueryCount = idx + 1;
        }
    }
#elif defined(RENDERER_OPENGL)
    if (mTimerQuerySupported)
    {
        glEndQuery(GL_TIME_ELAPSED);
        mUsedQueryCount = idx + 1;
    }
    else
    {
        mQueries[idx].fallbackEndTicks = SDL_GetPerformanceCounter();
        mQueries[idx].fallbackRecorded = true;
        mUsedQueryCount = idx + 1;
    }
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Query management
// ─────────────────────────────────────────────────────────────────────────────
uint32_t GPUProfiler::AllocQuery()
{
    // Queries are allocated sequentially within a frame. Because the pool is
    // reset at the start of every frame, index 0 is always free here.
    if (mNextQueryIndex >= mMaxQueries)
    {
        if (!mOverflowWarned)
        {
            std::cerr << "[GPUProfiler] Query pool full (" << mMaxQueries
                      << " draw calls); dropping profiling for the remaining draws this frame."
                      << std::endl;
            mOverflowWarned = true;
        }
        return UINT32_MAX;
    }

    const uint32_t idx = mNextQueryIndex++;
    mQueries[idx].active = true;
    mQueries[idx].fallbackRecorded = false;
    mActiveCount++;
    return idx;
}

void GPUProfiler::FreeQuery(uint32_t index)
{
    if (index < mMaxQueries && mQueries[index].active)
    {
        mQueries[index].active = false;
        if (mActiveCount > 0)
        {
            mActiveCount--;
        }
    }
}

void GPUProfiler::FlushQueryPool()
{
    // Kept for API compatibility; resolution is handled by ResolveQueries().
}

// ─────────────────────────────────────────────────────────────────────────────
// Query resolution
// ─────────────────────────────────────────────────────────────────────────────
void GPUProfiler::ResolveQueries()
{
    const uint32_t count = mUsedQueryCount;
    if (count == 0)
    {
        return;
    }

    mDrawCallSamples.clear();
    mFrameGPUTimeMs = 0.0;
    double totalMs = 0.0;

#ifdef RENDERER_VULKAN
    if (mQueryPool == VK_NULL_HANDLE)
    {
        mUsedQueryCount = 0;
        return;
    }

    rhi::vk::Device* vkDevice = rhi::vk::Device::Get();
    if (!vkDevice)
    {
        mUsedQueryCount = 0;
        return;
    }

    // The previous frame was submitted with the fence of the previous frame
    // index. Waiting on it guarantees the timestamps have been written.
    const uint32_t frameCount = static_cast<uint32_t>(BACKBUFFER_COUNT);
    const uint32_t prevFenceIdx = (static_cast<uint32_t>(vkDevice->GetCurrentFrameIndex()) + frameCount - 1) % frameCount;
    vkDevice->WaitForFenceAt(prevFenceIdx);

    uint64_t spanBegin = UINT64_MAX;
    uint64_t spanEnd = 0;

    for (uint32_t i = 0; i < count; ++i)
    {
        uint64_t timestamps[2] = { 0, 0 };

        // Read only this draw call's pair. Reading the whole pool would return
        // VK_NOT_READY because the slots after `count` were never written.
        const VkResult result = vkGetQueryPoolResults(
            mDevice, mQueryPool,
            i * 2, 2,
            sizeof(timestamps), timestamps,
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        // A non-monotonic pair means the query was not written; skip it. Equal
        // timestamps are legitimate: the GPU timestamp counter is coarse (often
        // ~1 us), so short draws complete within a single tick. Keep them with
        // 0 ms instead of hiding the draw call from the overlay.
        if (result != VK_SUCCESS || timestamps[1] < timestamps[0])
        {
            continue;
        }

        DrawCallSample sample;
        sample.objectName = mQueries[i].objectName;
        sample.shaderName = mQueries[i].shaderName;
        sample.materialName = mQueries[i].materialName;
        sample.instanceID = mQueries[i].instanceID;
        sample.gpuTimeMs = static_cast<double>(timestamps[1] - timestamps[0]) * mTimestampPeriod / 1e6;

        if (timestamps[0] < spanBegin) spanBegin = timestamps[0];
        if (timestamps[1] > spanEnd)   spanEnd = timestamps[1];

        mDrawCallSamples.push_back(std::move(sample));
    }

    // The frame's GPU time is the span covered by all draw calls. Summing the
    // per-draw deltas would lose every sub-tick draw.
    if (spanBegin != UINT64_MAX && spanEnd > spanBegin)
    {
        totalMs = static_cast<double>(spanEnd - spanBegin) * mTimestampPeriod / 1e6;
    }
#elif defined(RENDERER_OPENGL)
    // Make sure the GPU finished the previous frame's draws before reading the
    // query results (glGetQueryObject* would otherwise block unpredictably).
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    if (sync)
    {
        glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
        glDeleteSync(sync);
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        DrawCallSample sample;
        sample.objectName = mQueries[i].objectName;
        sample.shaderName = mQueries[i].shaderName;
        sample.materialName = mQueries[i].materialName;
        sample.instanceID = mQueries[i].instanceID;

        if (mTimerQuerySupported)
        {
            // GL_TIME_ELAPSED returns nanoseconds.
            uint64_t elapsedNs = 0;
            glGetQueryObjectui64v(mQueries[i].handle, GL_QUERY_RESULT, &elapsedNs);
            sample.gpuTimeMs = static_cast<double>(elapsedNs) / 1e6;
        }
        else if (mQueries[i].fallbackRecorded)
        {
            const uint64_t freq = SDL_GetPerformanceFrequency();
            const uint64_t ticks = mQueries[i].fallbackEndTicks - mQueries[i].fallbackBeginTicks;
            sample.gpuTimeMs = freq > 0
                ? static_cast<double>(ticks) / static_cast<double>(freq) * 1000.0
                : 0.0;
        }
        else
        {
            continue;
        }

        totalMs += sample.gpuTimeMs;
        mDrawCallSamples.push_back(std::move(sample));
    }
#endif

    mFrameGPUTimeMs = totalMs;
    mUsedQueryCount = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Vulkan backend
// ─────────────────────────────────────────────────────────────────────────────
#ifdef RENDERER_VULKAN
void GPUProfiler::InitVulkan()
{
    rhi::vk::Device* vkDevice = rhi::vk::Device::Get();
    if (!vkDevice)
    {
        return;
    }

    mDevice = vkDevice->GetDevice();
    VkPhysicalDevice physDev = vkDevice->GetPhysicalDevice();
    mCalibratedDevice = mDevice;

    // Timestamps are expressed in units of VkPhysicalDeviceLimits::timestampPeriod
    // nanoseconds (typically 1.0).
    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physDev, &props);
    mTimestampPeriod = props.limits.timestampPeriod > 0.0f ? props.limits.timestampPeriod : 1.0f;

    // ── VK_EXT_calibrated_timestamps (informational only) ─────────────────
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, extensions.data());

    bool hasCalibratedTimestamps = false;
    for (const auto& ext : extensions)
    {
        if (strcmp(ext.extensionName, VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME) == 0)
        {
            hasCalibratedTimestamps = true;
            break;
        }
    }

    if (hasCalibratedTimestamps)
    {
        mGetCalibrateableTimeDomainsEXT = reinterpret_cast<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>(
            vkGetInstanceProcAddr(vkDevice->GetVulkanInstance(), "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT"));
        mGetCalibratedTimestampsEXT = reinterpret_cast<PFN_vkGetCalibratedTimestampsEXT>(
            vkGetDeviceProcAddr(mDevice, "vkGetCalibratedTimestampsEXT"));

        if (mGetCalibrateableTimeDomainsEXT && mGetCalibratedTimestampsEXT)
        {
            mCalibratedTimestampsSupported = true;
        }
    }

    // Each draw call uses 2 timestamps (begin + end).
    const uint32_t queryCount = mMaxQueries * 2;
    VkQueryPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolInfo.queryCount = queryCount;
    poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;

    VKCall(vkCreateQueryPool(mDevice, &poolInfo, nullptr, &mQueryPool), "Query pool creation failed");
}

void GPUProfiler::CloseVulkan()
{
    // The query pool must be destroyed while the VkDevice is still alive.
    // Engine::Stop() closes the profiler before the renderer for this reason.
    if (mDevice != VK_NULL_HANDLE && mQueryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(mDevice, mQueryPool, nullptr);
    }
    mQueryPool = VK_NULL_HANDLE;
    mDevice = VK_NULL_HANDLE;

    mCalibratedTimestampsSupported = false;
    mGetCalibrateableTimeDomainsEXT = nullptr;
    mGetCalibratedTimestampsEXT = nullptr;
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// OpenGL backend
// ─────────────────────────────────────────────────────────────────────────────
#ifdef RENDERER_OPENGL
void GPUProfiler::InitOpenGL()
{
    // Core profile (GL 3.2+) returns nullptr from glGetString(GL_EXTENSIONS),
    // so enumerate via glGetStringi first.
    bool hasTimerQuery = false;

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (GLint i = 0; i < numExtensions; ++i)
    {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        if (ext && (strstr(ext, "GL_EXT_timer_query") || strstr(ext, "GL_ARB_timer_query")))
        {
            hasTimerQuery = true;
            break;
        }
    }

    if (!hasTimerQuery)
    {
        return;
    }

    mTimerQuerySupported = true;

    for (uint32_t i = 0; i < mMaxQueries; ++i)
    {
        glGenQueries(1, &mQueries[i].handle);
    }
}

void GPUProfiler::CloseOpenGL()
{
    if (!mQueries.empty())
    {
        for (uint32_t i = 0; i < mMaxQueries; ++i)
        {
            if (mQueries[i].handle != 0)
            {
                glDeleteQueries(1, &mQueries[i].handle);
                mQueries[i].handle = 0;
            }
        }
    }
    mTimerQuerySupported = false;
}
#endif
