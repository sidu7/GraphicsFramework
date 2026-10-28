#pragma once

// ProfilerOverlay.h — ImGui overlay for profiler data visualization.

#include "Profiling/ProfilerInternal.h"

class Profiler;  // Forward declaration; full include in .cpp

class ProfilerOverlay
{
public:
    void Render(const Profiler& profiler);
    void SetVisible(bool visible) { mVisible = visible; }
    bool IsVisible() const { return mVisible; }

    // Configuration
    void SetWindowPosition(int x, int y) { mPosX = x; mPosY = y; }
    void SetWindowSize(int w, int h) { mWinW = w; mWinH = h; }

private:
    // Sub-panels
    void RenderFrameGraph(const FrameSnapshot& snapshot);
    void RenderCPUSamples(const std::vector<CPUSample>& samples);
    void RenderDrawCalls(const std::vector<DrawCallSample>& drawCalls);
    void RenderExportButton(Profiler& profiler);

private:
    bool mVisible = false;
    int  mPosX = 10;
    int  mPosY = 10;
    int  mWinW = 420;
    int  mWinH = 580;
};
