// ProfilerOverlay.cpp — ImGui overlay for profiler visualization.
// Reads from the unified FrameSnapshot provided by Profiler, not from
// individual subsystem profilers directly.

#include "Profiling/ProfilerOverlay.h"
#include "Profiling/Profiler.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <Imgui/imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

// ─────────────────────────────────────────────────────────────────────────────
// Render
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::Render(const Profiler& profiler)
{
    if (!profiler.IsEnabled()) return;

    const auto& snapshot = profiler.GetSnapshot();

    ImGuiIO& io = ImGui::GetIO();

    //ImGui::SetNextWindowPos(ImVec2(static_cast<float>(mPosX), static_cast<float>(mPosY)));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(mWinW), static_cast<float>(mWinH)));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Profiler", nullptr, flags);

    // Frame time graph (CPU-side history)
    RenderFrameGraph(snapshot);

    ImGui::SeparatorText("CPU Samples (grouped)");
    RenderCPUSamples(snapshot.cpuSamples);

    ImGui::SeparatorText("Draw Calls");
    RenderDrawCalls(snapshot.drawCalls);

    ImGui::Separator();
    RenderExportButton(const_cast<Profiler&>(profiler));

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame time graph — rolling bar chart
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::RenderFrameGraph(const FrameSnapshot& snapshot)
{
    const auto& history = snapshot.frameTimeHistory;
    if (history.empty()) return;

    ImGui::Text("Frame Time (last %zu frames)", history.size());

    // Calculate max for scaling
    float maxTime = 1.0f;
    for (float t : history)
    {
        if (t > maxTime) maxTime = t;
    }
    maxTime *= 1.1f; // 10% headroom

    // Draw horizontal bar chart
    float barHeight = 16.0f;
    float spacing = 2.0f;
    float totalHeight = history.size() * (barHeight + spacing);

    // Show in reverse (newest at bottom)
    ImVec2 windowSize = ImGui::GetWindowSize();
    float graphWidth = windowSize.x - 20.0f;

    for (int i = static_cast<int>(history.size()) - 1; i >= 0; --i)
    {
        float time = static_cast<float>(history[static_cast<size_t>(i)]);
        float barWidth = (time / maxTime) * graphWidth;

        ImVec2 pos(10.0f, 10.0f + static_cast<float>(i) * (barHeight + spacing));
        ImVec2 size(barWidth, barHeight);

        // Color based on time (green → yellow → red)
        ImU32 color;
        float ratio = time / maxTime;
        if (ratio < 0.5f)
            color = IM_COL32(80, 200, 80, 255);  // Green
        else if (ratio < 0.8f)
            color = IM_COL32(230, 200, 50, 255); // Yellow
        else
            color = IM_COL32(230, 60, 60, 255);  // Red

        ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + size, color, 2.0f);

        // Time label
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << time << "ms";
        std::string label = oss.str();
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        ImGui::GetWindowDrawList()->AddText(
            pos + ImVec2(size.x + 4.0f, barHeight / 2.0f - textSize.y / 2.0f),
            IM_COL32(200, 200, 200, 255),
            label.c_str()
        );
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CPU samples table
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::RenderCPUSamples(const std::vector<CPUSample>& samples)
{
    if (samples.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No samples recorded");
        return;
    }

    // Show top 50 samples
    size_t displayCount = std::min(samples.size(), static_cast<size_t>(50));

    if (ImGui::BeginTable("CPUSamples", 4, 0, ImVec2(0, -ImGui::GetTextLineHeightWithSpacing())))
    {
        ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 60.0f, 1);
        ImGui::TableSetupColumn("Total (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f, 2);
        ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f, 3);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < displayCount; ++i)
        {
            const auto& sample = samples[i];

            // Truncate long names
            std::string displayName = sample.name;
            if (displayName.size() > 60)
            {
                displayName = displayName.substr(0, 57) + "...";
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(displayName.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", sample.count);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", sample.totalTime);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", sample.avgTime);
        }

        if (samples.size() > 50)
        {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f),
                "... and %zu more samples (showing top 50)", samples.size() - 50);
        }

        ImGui::EndTable();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw calls table
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::RenderDrawCalls(const std::vector<DrawCallSample>& drawCalls)
{
    if (drawCalls.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No draw call data");
        return;
    }

    ImGui::Text("%zu draw calls", drawCalls.size());

    // Show in a scrollable list
    ImGui::BeginChild("DrawCallsList", ImVec2(0, -30.0f), true);

    for (const auto& draw : drawCalls)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << draw.gpuTimeMs << "ms";
        std::string timeStr = oss.str();

        // Format: [ID] object (shader) — material — time
        std::string label = "[" + std::to_string(draw.instanceID) + "] ";
        if (!draw.objectName.empty())
            label += draw.objectName + " ";
        else
            label += "(no name) ";

        if (!draw.shaderName.empty())
            label += "(" + draw.shaderName + ") ";

        if (!draw.materialName.empty())
            label += "— " + draw.materialName;

        label += " — " + timeStr;

        ImGui::TextUnformatted(label.c_str());
    }

    ImGui::EndChild();
}

// ─────────────────────────────────────────────────────────────────────────────
// Export button
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::RenderExportButton(Profiler& profiler)
{
    if (!&profiler) return;

    static int exportCounter = 0;
    static bool justExported = false;

    if (ImGui::Button("Export CSV"))
    {
        exportCounter++;
        std::ostringstream oss;
        oss << "profiler_frame_" << std::setw(3) << std::setfill('0') << exportCounter << ".csv";
        std::string filepath = oss.str();

        // Create output directory if it doesn't exist
        std::filesystem::path path(filepath);
        if (path.has_parent_path())
        {
            std::filesystem::create_directories(path.parent_path());
        }

        if (profiler.ExportCSV(filepath))
        {
            justExported = true;
        }
    }

    if (justExported)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Exported!");
        justExported = false;
    }

    ImGui::SameLine();
    ImGui::Text("Frame: %u", profiler.GetFrameCount());
}
