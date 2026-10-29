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
    //ImGui::SetNextWindowSize(ImVec2(static_cast<float>(mWinW), static_cast<float>(mWinH)));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(620, 720), ImGuiCond_FirstUseEver);
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

    // Show at most the last 60 frames so the chart stays readable.
    const size_t showCount = std::min<size_t>(history.size(), 60);
    std::vector<float> values;
    values.reserve(showCount);
    for (size_t i = history.size() - showCount; i < history.size(); ++i)
    {
        values.push_back(static_cast<float>(history[i]));
    }

    float maxTime = 1.0f;
    for (float t : values)
    {
        if (t > maxTime) maxTime = t;
    }
    maxTime *= 1.1f; // 10% headroom

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f | Frame: %.2f ms | CPU: %.2f ms | GPU: %.2f ms",
        io.Framerate, snapshot.frameTimeMs, snapshot.cpuTimeMs, snapshot.gpuTimeMs);
    ImGui::Text("CPU Render: %.2f ms | Update: %.2f ms | ImGui: %.2f ms | Wait: %.2f ms",
        snapshot.renderLoopTimeMs, snapshot.updateLoopTimeMs,
        snapshot.imguiTimeMs, snapshot.waitTimeMs);
    ImGui::Text("Draw Calls: %zu", snapshot.drawCalls.size());

    ImGui::Text("Frame Time (last %zu frames)", values.size());
    ImGui::PlotHistogram("##FrameTime", values.data(), static_cast<int>(values.size()),
        0, nullptr, 0.0f, maxTime, ImVec2(0, 80));
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

    if (ImGui::BeginTable("CPUSamples", 4,
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV,
            ImVec2(0, 160.0f)))
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

    // Fixed height: a negative size collapses to nothing because the Profiler
    // window auto-sizes to its content.
    ImGui::BeginChild("DrawCallsList", ImVec2(0, 240.0f), true);

    for (const auto& draw : drawCalls)
    {
        std::ostringstream oss;
        if (draw.gpuTimeMs > 0.0)
        {
            oss << std::fixed << std::setprecision(3) << draw.gpuTimeMs << "ms";
        }
        else
        {
            oss << "<0.001ms";
        }
        std::string timeStr = oss.str();

        // Format: [ID] object [shader] - material - time
        std::string label = "[" + std::to_string(draw.instanceID) + "] ";

        if (!draw.objectName.empty())
            label += draw.objectName + " ";

        if (!draw.shaderName.empty())
            label += "[" + draw.shaderName + "] ";

        if (!draw.materialName.empty())
            label += draw.materialName + " ";

        label += " - " + timeStr;

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
