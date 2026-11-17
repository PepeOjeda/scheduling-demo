#pragma once
#include <cstddef>
#include <string>

enum class TaskStatus
{
    Queued,
    Running,
    Completed
};

struct Task
{
    std::size_t id = 0;
    std::string name;
    int priority = 0;

    double timeAdded = 0.0;
    double remainingDuration = 0.0;
    double lastExecutionTime = 0.0;

    // used for drawing the GUI
    TaskStatus status = TaskStatus::Queued;
};