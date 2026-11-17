#include "Scheduler.hpp"
#include "imgui_gl/fonts.hpp"
#include "imgui_gl/imgui_gl.h"
#include "implot/implot.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

Scheduler::Scheduler()
{
    AddTask("T1", 2, 150);
    AddTask("T2", 1, 100);
    AddTask("T3", 3, 50);

    SortTasks();
}

static ImFont* fontBody = nullptr;
static ImFont* fontHeader = nullptr;

void Scheduler::Run()
{
    ImguiGL::Setup(nullptr, "Task scheduling demo", 1100, 800);

    // load fonts
    {
        // avoid an invalid free() on shutdown
        ImFontConfig* cfg = new ImFontConfig();
        cfg->FontDataOwnedByAtlas = false;

        fontBody = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)ImGui::Fonts::Roboto_Variable, ImGui::Fonts::Roboto_Variable_size, 18.f, cfg);
        fontHeader = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)ImGui::Fonts::Roboto_Variable, ImGui::Fonts::Roboto_Variable_size, 20.f, cfg);
        ImGui::GetIO().Fonts->Build();
    }
    ImGui::PushFont(fontBody);

    ImPlot::CreateContext();
    while (!ImguiGL::ShouldClose())
    {
        ImguiGL::StartFrame();
        Render();
        ImguiGL::Render();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    ImPlot::DestroyContext();
    ImguiGL::Close();
}

void Scheduler::AddTask(const std::string& name, int priority, double duration)
{
    Task task;
    task.id = tasks.size() + 1;
    task.name = name;
    task.timeAdded = currentTime;
    task.priority = priority;
    task.remainingDuration = duration;
    task.lastExecutionTime = currentTime;
    tasks.push_back(task);
    queuedTaskIds.push_back(task.id);
}
void Scheduler::ChooseNextTask()
{
    // choose which task gets to run
    SortTasks();

    // if the first task in the sorted queue is different from the one that was already running
    if (runningTaskId != queuedTaskIds.front())
    {
        // PREEMPTION!
        runningTaskId = queuedTaskIds.front();
        Task& task = tasks[runningTaskId - 1];
        task.status = TaskStatus::Running;
        executions.push_back({task.id, currentTime, currentTime});
    }
}

void Scheduler::StepSimulation()
{
    constexpr double deltaMs = 25.0;

    if (queuedTaskIds.empty())
    {
        currentTime += deltaMs;
        return;
    }

    ChooseNextTask();

    // simulate a step
    currentTime += deltaMs;
    Task& task = tasks[runningTaskId - 1];
    task.remainingDuration -= deltaMs;
    task.lastExecutionTime = currentTime;
    
    // Update the execution history (for GUI)
    executions.back().end = currentTime;

    // if the task is done, remove it from the queue
    if (task.remainingDuration <= 0)
    {
        task.remainingDuration = 0.0;
        task.status = TaskStatus::Completed;
        queuedTaskIds.erase(queuedTaskIds.begin());
    }
}

static bool ComparePriority(const Task& left, const Task& right)
{
    return left.priority > right.priority;
}

void Scheduler::SortTasks()
{
    std::stable_sort(queuedTaskIds.begin(), queuedTaskIds.end(), [this](std::size_t leftId, std::size_t rightId)
                     {
                         const Task& left = tasks[leftId - 1];
                         const Task& right = tasks[rightId - 1];

                         return ComparePriority(left, right);
                     });
}

// ----------------------------
// ----------------------------
// GUI Rendering
// ----------------------------
// ----------------------------

void Scheduler::Render()
{
    ImguiGL::SetNextWindowFullscreen();
    ImGui::Begin("Scheduler dashboard", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoTitleBar);

    ImGui::Text("SCHEDULER");
    ImGui::SameLine();
    ImGui::TextDisabled("Priority queue  /  single worker");
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::Text("Elapsed  %.1f ms", currentTime);
    ImGui::SameLine(0.0f, 18.0f);
    ImGui::Text("Queued  %zu", queuedTaskIds.size());
    ImGui::SameLine(0.0f, 18.0f);
    ImGui::Text("Completed  %zu", std::count_if(tasks.begin(), tasks.end(), [](const Task& task)
                                                {
                                                    return task.status == TaskStatus::Completed;
                                                }));

    if (ImGui::Button("Step 25 ms"))
        StepSimulation();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("Task name", newTaskName, sizeof(newTaskName));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::InputInt("Priority", &newTaskPriority);
    newTaskPriority = std::max(0, newTaskPriority);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    ImGui::DragFloat("Duration (ms)", &newTaskDuration, 0.1f, 0.1f, 500.0f, "%.1f");
    newTaskDuration = std::max(0.1f, std::min(newTaskDuration, 500.0f));
    ImGui::SameLine();
    if (ImGui::Button("Add task") && newTaskName[0] != '\0')
    {
        AddTask(newTaskName, newTaskPriority, newTaskDuration);
        SortTasks();
    }

    ImGui::Separator();
    ImGui::Text("TASK QUEUE");
    ImGui::SameLine();
    if (ImGui::BeginTable("Task queue", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp,
                          ImVec2(0.0f, 170.0f)))
    {
        ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Task");
        ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Remaining duration", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableHeadersRow();
        for (std::size_t i = 0; i < queuedTaskIds.size(); ++i)
        {
            const Task& task = tasks[queuedTaskIds[i] - 1];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", i + 1);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", task.name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", task.priority);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1f ms", task.remainingDuration);
        }
        if (queuedTaskIds.empty())
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("No tasks waiting");
        }
        ImGui::EndTable();
    }

    ImGui::Separator();

    // running task info
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(42, 91, 72, 255));
        ImGui::PushFont(fontHeader);
        ImGui::BeginChild("RunningTaskHighlight", ImVec2(0.0f, 42.0f), true);
        if (runningTaskId != 0)
        {
            const Task& runningTask = tasks[runningTaskId - 1];
            ImGui::Text("RUNNING  %s  |  priority %d  |  %.1f ms remaining",
                        runningTask.name.c_str(), runningTask.priority, runningTask.remainingDuration);
        }
        else
        {
            ImGui::TextDisabled("Worker idle - press Step to advance");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    ImGui::Separator();
    ImGui::Text("EXECUTION TIMELINE");
    ImGui::SameLine();
    if (ImPlot::BeginPlot("##ExecutionTimeline", ImVec2(-1.0f, -1.0f),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus))
    {
        ImPlot::SetupAxes("Elapsed time (s)", "Task");
        const double timeLimit = std::max(10.0, currentTime * 1.05);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, timeLimit, ImPlotCond_Always);
        if (!tasks.empty())
        {
            std::vector<double> laneValues;
            std::vector<const char*> laneLabels;
            laneValues.reserve(tasks.size());
            laneLabels.reserve(tasks.size());
            for (std::size_t i = 0; i < tasks.size(); ++i)
            {
                laneValues.push_back(static_cast<double>(i));
                laneLabels.push_back(tasks[i].name.c_str());
            }
            ImPlot::SetupAxisTicks(ImAxis_Y1, laneValues.data(), static_cast<int>(laneValues.size()), laneLabels.data());
            ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, static_cast<double>(tasks.size()) - 0.5, ImPlotCond_Always);

            ImPlot::PushPlotClipRect();
            ImDrawList* drawList = ImPlot::GetPlotDrawList();
            for (const Execution& execution : executions)
            {
                const Task& task = tasks[execution.taskId - 1];
                const double lane = static_cast<double>(execution.taskId - 1);
                const ImVec2 start = ImPlot::PlotToPixels(execution.start, lane - 0.32);
                const ImVec2 end = ImPlot::PlotToPixels(execution.end, lane + 0.32);
                const ImVec2 min(std::min(start.x, end.x), std::min(start.y, end.y));
                const ImVec2 max(std::max(start.x, end.x), std::max(start.y, end.y));
                const ImVec4 taskColor = ImColor::HSV(std::fmod(task.id * 0.17f, 1.0f), 0.62f, 0.88f);
                const ImU32 color = task.status == TaskStatus::Running
                                        ? IM_COL32(88, 194, 146, 255)
                                        : ImGui::GetColorU32(taskColor);
                drawList->AddRectFilled(min, max, color, 3.0f);
                if (max.x - min.x > 54.0f)
                {
                    const ImVec2 textSize = ImGui::CalcTextSize(task.name.c_str());
                    drawList->AddText(ImVec2((min.x + max.x - textSize.x) * 0.5f,
                                             (min.y + max.y - textSize.y) * 0.5f),
                                      IM_COL32(255, 255, 255, 255), task.name.c_str());
                }
            }
            ImPlot::PopPlotClipRect();
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
}
