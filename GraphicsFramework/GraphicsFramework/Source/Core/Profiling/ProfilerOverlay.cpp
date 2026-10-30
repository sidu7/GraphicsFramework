// ProfilerOverlay.cpp — ImGui overlay for profiler visualization.
// Reads from the unified FrameSnapshot provided by Profiler, not from
// individual subsystem profilers directly.

#include "Core/Profiling/ProfilerOverlay.h"
#include "Core/Profiling/Profiler.h"

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

    ImGui::SeparatorText("CPU Samples");
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
// CPU samples — hierarchical tree view (parent scopes expand to show children)
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::RenderCPUSamples(const std::vector<CPUSample>& samples)
{
    if (samples.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No samples recorded");
        return;
    }

    // Helper: does this sample have any children?
    auto hasChildren = [&](int index)
    {
        for (const auto& s : samples)
            if (s.parentIndex == index) return true;
        return false;
    };

    // Collect root-level samples (parentIndex == -1).
    std::vector<const CPUSample*> roots;
    roots.reserve(samples.size());
    for (const auto& s : samples)
    {
        if (s.parentIndex == -1)
            roots.push_back(&s);
    }

    if (!roots.empty())
    {
        // Render each root as a top-level tree node, then recurse into children.
        for (const auto* root : roots)
        {
            int rootIdx = static_cast<int>(
                std::distance(samples.begin(),
                    std::find_if(samples.begin(), samples.end(),
                        [root](const CPUSample& s) { return &s == root; }))
            );

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << root->totalTime
                << " ms  (" << root->count << "x)";
            std::string label = root->name + "  " + oss.str();

            if (hasChildren(rootIdx))
            {
                std::string idText = "###cpu_root_" + std::to_string(rootIdx);
                bool opened = ImGui::TreeNodeEx(idText.c_str(),
                    ImGuiTreeNodeFlags_None, "%s", label.c_str());
                if (opened)
                {
                    RenderTreeRecursive(samples, rootIdx, 1);
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::Bullet();
                ImGui::TextUnformatted(label.c_str());
            }
        }
    }
    else
    {
        // Fallback: no root-level samples, render all as a flat list.
        for (const auto& sample : samples)
        {
            int idx = static_cast<int>(
                std::distance(samples.begin(),
                    std::find_if(samples.begin(), samples.end(),
                        [&sample](const CPUSample& s) { return &s == &sample; }))
            );

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << sample.totalTime
                << " ms  (" << sample.count << "x)";
            std::string label = sample.name + "  " + oss.str();

            if (hasChildren(idx))
            {
                std::string idText = "###cpu_flat_" + std::to_string(idx);
                bool opened = ImGui::TreeNodeEx(idText.c_str(), 0, "%s", label.c_str());
                if (opened)
                {
                    RenderTreeRecursive(samples, idx, 1);
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::Bullet();
                ImGui::TextUnformatted(label.c_str());
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Recursive tree rendering for child scopes
// ─────────────────────────────────────────────────────────────────────────────
void ProfilerOverlay::RenderTreeRecursive(
    const std::vector<CPUSample>& samples, int parentIndex, int depth)
{
    auto hasChildren = [&](int index)
    {
        for (const auto& s : samples)
            if (s.parentIndex == index) return true;
        return false;
    };

    for (const auto& sample : samples)
    {
        if (sample.parentIndex == parentIndex)
        {
            int idx = static_cast<int>(
                std::distance(samples.begin(),
                    std::find_if(samples.begin(), samples.end(),
                        [&sample](const CPUSample& s) { return &s == &sample; }))
            );

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << sample.totalTime
                << " ms  (" << sample.count << "x)";
            std::string label = sample.name + "  " + oss.str();

            if (hasChildren(idx))
            {
                ImGui::Indent(depth * 4.0f);
                ImGui::PushID(idx);

                std::string idText = "###cpu_child_" + std::to_string(idx);
                bool opened = ImGui::TreeNodeEx(idText.c_str(), 0, "%s", label.c_str());

                if (opened)
                {
                    RenderTreeRecursive(samples, idx, depth + 1);
                    ImGui::TreePop();
                }

                ImGui::PopID();
                ImGui::Unindent(depth * 4.0f);
            }
            else
            {
                ImGui::Indent(depth * 4.0f);
                ImGui::Bullet();
                ImGui::TextUnformatted(label.c_str());
                ImGui::Unindent(depth * 4.0f);
            }
        }
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
