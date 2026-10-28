// GPUProfiler.cpp — GPU profiling via query pools.
// Backend dispatch: RENDERER_VULKAN → Vulkan timestamps, RENDERER_OPENGL → GL_ARB_timer_query.

#include "Profiling/GPUProfiler.h"

#include <algorithm>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// Common interface
// ─────────────────────────────────────────────────────────────────────────────
void GPUProfiler::Init(uint32_t maxQueries)
{
    mMaxQueries = maxQueries;
    mQueries.resize(maxQueries);

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
}

void GPUProfiler::StartFrame()
{
    mFrameGPUTimeMs = 0.0;
    mDrawCallSamples.clear();
    mNextDrawInstance = 0;

    // Reset query pool
#ifdef RENDERER_VULKAN
    vkResetQueryPool(mDevice, mQueryPool, 0, mMaxQueries);
#endif

    mActiveCount = 0;
    for (uint32_t i = 0; i < mMaxQueries; ++i)
    {
        mQueries[i].active = false;
    }
}

void GPUProfiler::EndFrame()
{
    // Resolve any pending queries
    ResolveQueries();
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw call profiling
// ─────────────────────────────────────────────────────────────────────────────
void GPUProfiler::BeginDrawCall(const std::string& objectName, const std::string& shaderName, const std::string& materialName)
{
    // Flush if pool is full
    if (mActiveCount >= mMaxQueries)
    {
        FlushQueryPool();
    }

    uint32_t idx = AllocQuery();
    if (idx == UINT32_MAX) return;

    mQueries[idx].objectName = objectName;
    mQueries[idx].shaderName = shaderName;
    mQueries[idx].materialName = materialName;
    mQueries[idx].instanceID = mNextDrawInstance++;

#ifdef RENDERER_VULKAN
    // Emit timestamps
    static VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    // Note: In practice, we'd record into a per-frame command buffer.
    // For simplicity, we emit the begin/end into a recorded command buffer
    // that gets submitted at the end of the frame.
    // This is a simplified version — production code would need proper
    // command buffer management.
    (void)idx; // Query emit happens via renderer hooks
#elif defined(RENDERER_OPENGL)
    if (mTimerQuerySupported && mQueries[idx].active)
    {
        glBeginQuery(GL_TIME_ELAPSED_EXT, mQueries[idx].handle);
    }
#endif
}

void GPUProfiler::EndDrawCall()
{
#ifdef RENDERER_OPENGL
    if (mTimerQuerySupported)
    {
        glEndQuery(GL_TIME_ELAPSED_EXT);
    }
#endif
    (void)0; // Vulkan end is implicit — query is resolved later
}

// ─────────────────────────────────────────────────────────────────────────────
// Query management
// ─────────────────────────────────────────────────────────────────────────────
uint32_t GPUProfiler::AllocQuery()
{
    for (uint32_t i = 0; i < mMaxQueries; ++i)
    {
        uint32_t idx = (mNextQueryIndex + i) % mMaxQueries;
        if (!mQueries[idx].active)
        {
            mQueries[idx].active = true;
            mActiveCount++;
            return idx;
        }
    }
    return UINT32_MAX; // Pool full — caller should flush
}

void GPUProfiler::FreeQuery(uint32_t index)
{
    if (index < mMaxQueries && mQueries[index].active)
    {
        mQueries[index].active = false;
        mActiveCount--;
    }
}

void GPUProfiler::FlushQueryPool()
{
    // Mark all queries as inactive and resolve them
    for (uint32_t i = 0; i < mMaxQueries; ++i)
    {
        if (mQueries[i].active)
        {
#ifdef RENDERER_OPENGL
            if (mTimerQuerySupported)
            {
                uint64_t elapsed = 0;
                glGetQueryObjectui64vEXT(mQueries[i].handle, GL_QUERY_RESULT_EXT, &elapsed);
                
                DrawCallSample sample;
                sample.objectName = mQueries[i].objectName;
                sample.shaderName = mQueries[i].shaderName;
                sample.materialName = mQueries[i].materialName;
                sample.gpuTimeMs = elapsed / 1e6; // Convert ns to ms
                sample.instanceID = mQueries[i].instanceID;
                mDrawCallSamples.push_back(sample);
            }
#endif
            FreeQuery(i);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Query resolution
// ─────────────────────────────────────────────────────────────────────────────
void GPUProfiler::ResolveQueries()
{
    // In a real implementation, this would be called after the GPU has finished
    // processing all queries. For Vulkan, this requires vkQueryResult with
    // VK_QUERY_RESULT_WAIT_BIT or using an fence to ensure completion.
    // For OpenGL, glGetQueryObjectui64vEXT is used.
    
    // Simplified: samples are collected during FlushQueryPool when queries
    // are freed. In production, you'd submit a resolve command buffer / read
    // query results synchronously.
}

// ─────────────────────────────────────────────────────────────────────────────
// Vulkan backend
// ─────────────────────────────────────────────────────────────────────────────
#ifdef RENDERER_VULKAN
void GPUProfiler::InitVulkan()
{
    // TODO: Initialize Vulkan query pool
    // - Get VkDevice from renderer
    // - Query supported timestamp features (VK_EXT_calibrated_timestamps)
    // - Create VkQueryPool with VK_QUERY_TYPE_TIMESTAMP
    // - Get timestamp period via vkGetPhysicalDeviceProperties2
}

void GPUProfiler::CloseVulkan()
{
    if (mQueryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(mDevice, mQueryPool, nullptr);
        mQueryPool = VK_NULL_HANDLE;
    }
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// OpenGL backend
// ─────────────────────────────────────────────────────────────────────────────
#ifdef RENDERER_OPENGL
void GPUProfiler::InitOpenGL()
{
    // Check for GL_EXT_timer_query or GL_ARB_timer_query support
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!extensions)
    {
        std::cerr << "[GPUProfiler] No extension string available" << std::endl;
        return;
    }

    bool hasTimerQuery = strstr(extensions, "GL_EXT_timer_query") ||
                         strstr(extensions, "GL_ARB_timer_query");

    if (!hasTimerQuery)
    {
        std::cerr << "[GPUProfiler] GL_EXT_timer_query / GL_ARB_timer_query not supported" << std::endl;
        return;
    }

    mTimerQuerySupported = true;

    // Generate query handles
    for (uint32_t i = 0; i < mMaxQueries; ++i)
    {
        glGenQueriesEXT(1, &mQueries[i].handle);
    }
}

void GPUProfiler::CloseOpenGL()
{
    if (mTimerQuerySupported)
    {
        for (uint32_t i = 0; i < mMaxQueries; ++i)
        {
            if (mQueries[i].active)
            {
                glDeleteQueriesEXT(1, &mQueries[i].handle);
            }
        }
        mQueries.clear();
    }
}
#endif
