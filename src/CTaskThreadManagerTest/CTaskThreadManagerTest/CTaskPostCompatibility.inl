#pragma once

template <typename Thread>
auto CTaskPostCompatibility::submit(Thread& thread, const std::shared_ptr<CTask>& task, int32_t level, int)
    -> decltype(thread.postTaskResult(task, level))
{
    return thread.postTaskResult(task, level);
}

template <typename Thread>
auto CTaskPostCompatibility::submit(Thread& thread, const std::shared_ptr<CTask>& task, int32_t level, long)
    -> decltype(thread.postTask(task, level))
{
    return thread.postTask(task, level);
}