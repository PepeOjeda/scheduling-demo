#pragma once
#include "Task.hpp"
#include <cstddef>
#include <string>
#include <vector>

class Scheduler
{
public:
    Scheduler();
    void Run();

private:
    struct Execution
    {
        std::size_t taskId;
        double start;
        double end;
    };

    std::vector<Task> tasks;
    std::vector<std::size_t> queuedTaskIds;
    std::vector<Execution> executions;
    std::size_t runningTaskId = 0;
    double currentTime = 0.0;

    char newTaskName[64] = "New task";
    int newTaskPriority = 2;
    float newTaskDuration = 50.0f;

    void AddTask(const std::string& name, int priority, double duration);
    void ChooseNextTask();
    void StepSimulation();
    void SortTasks();
    void Render();
};