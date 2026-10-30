#pragma once

// ProfilerInternal.h — Internal profiler data structures.
// This header defines shared types and the unified FrameSnapshot.
// It is included by CPUProfiler, GPUProfiler, Profiler, and ProfilerOverlay
// but NOT exposed to application code.

#include <string>
#include <vector>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// CPU sample — represents one unique profiled function/scope invocation.
// Multiple calls with the same name are grouped together.
// Owned by CPUProfiler.
// ─────────────────────────────────────────────────────────────────────────────
struct CPUSample
{
    std::string  name;        // Function/scope name (file:line or custom label)
    uint32_t     count;       // How many times this scope was entered
    double       totalTime;   // Cumulative time across all invocations (ms)
    double       avgTime;     // totalTime / count (ms)

    // Hierarchy — set during GroupSamples().
    // parentIndex is the index into mSamples vector, or -1 if root-level.
    int          parentIndex = -1;
    // Full scope path captured during sampling (used to build hierarchy).
    std::string  path;
};

// ─────────────────────────────────────────────────────────────────────────────
// Draw call sample — one entry per drawn object / draw call.
// Owned by GPUProfiler.
// ─────────────────────────────────────────────────────────────────────────────
struct DrawCallSample
{
    std::string  objectName;   // Object name from Shape component (if available)
    std::string  shaderName;   // Shader name/ID
    std::string  materialName; // Material name (from diffuse color or explicit name)
    double       gpuTimeMs;    // GPU execution time for this draw call (ms)
    uint32_t     instanceID;   // Unique draw call sequence number
};

// ─────────────────────────────────────────────────────────────────────────────
// GPU query result — one entry per enqueued GPU query.
// Internal to GPUProfiler; not part of the public snapshot.
// ─────────────────────────────────────────────────────────────────────────────
struct GPUQueryResult
{
    uint64_t startTimestamp = 0;   // Nanoseconds (or GL query result)
    uint64_t endTimestamp   = 0;
    double   elapsedMs      = 0.0;
    std::string objectName;
    std::string shaderName;
    std::string materialName;
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-frame snapshot — the unified data structure consumed by the overlay,
// CSV exporter, and any external tooling. Composed by Profiler::EndFrame()
// from CPUProfiler + GPUProfiler subsystem data.
// ─────────────────────────────────────────────────────────────────────────────
struct FrameSnapshot
{
    double   frameTimeMs;          // Total frame time (wall clock)
    double   cpuTimeMs;            // CPU-side frame time (engine loop)
    double   gpuTimeMs;            // GPU-side frame time (query pool result)
    double   renderLoopTimeMs;     // Time spent in the render loop
    double   updateLoopTimeMs;     // Time spent in the update loop
    double   imguiTimeMs;          // Time spent in ImGui rendering
    double   waitTimeMs;           // Time waiting for GPU (swap / vsync)

    // CPU and GPU data are kept as separate fields for clarity.
    std::vector<CPUSample>       cpuSamples;
    std::vector<DrawCallSample>  drawCalls;

    // Rolling history for frame-time graph (last N frames)
    static constexpr int MaxHistory = 120;
    std::vector<double> frameTimeHistory;
};

// ─────────────────────────────────────────────────────────────────────────────
// Profiler settings — parsed from Settings.json
// ─────────────────────────────────────────────────────────────────────────────
struct ProfilerSettings
{
    uint32_t maxConcurrentQueries    = 1024;  // Max GPU query pool size
    bool     enabled                 = true;
    bool     captureDrawCalls        = true;
    double   maxCPUSampleDurationMs  = 0.0;  // 0 = capture all (filter threshold)
};
