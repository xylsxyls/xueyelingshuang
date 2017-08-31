#include "CTaskThread.h"
#include <assert.h>

CTaskThread::CTaskThread(__int32 threadId) :
m_threadId(threadId)
{

}

bool CTaskThread::CreateThread()
{
    //必须先将m_hasExitSignal设为false，否则一进入线程函数就退出了
    std::thread* pThread = new std::thread(std::bind(&CTaskThread::WorkThread, this));
    if (pThread == nullptr)
    {
        m_hasExitSignal = true;
        return false;
    }
    m_spWorkThread.reset(pThread);
    return true;
}

void CTaskThread::DeleteThread()
{
    //如果线程正常启动
    if (m_spWorkThread != NULL)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_hasExitSignal = true;
            StopAllTaskUnlock();
        }
        m_spWorkThread->join();
        
        std::map<__int32, __int32> taskCountMap;
        GetWaitTaskCount(taskCountMap);
        m_spWorkThread.reset(NULL);
    }
}

void CTaskThread::PostTask(const std::shared_ptr<CTask>& task, __int32 taskLevel)
{
    if (task == NULL || taskLevel < 1 || m_hasExitSignal)
    {
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_hasExitSignal)
    {
        return;
    }
    m_taskMap[taskLevel].push_back(task);
    //如果添加任务的优先级高于当前任务则当前任务停止
    if (m_spCurTask != NULL && taskLevel > m_curTaskLevel)
    {
        //如果该任务的属性是被顶掉后重做，则先添加此任务，判空是防止当前任务还没来得及退出就添加了多个优先级更高的任务
        if (m_spCurTaskBk != NULL && m_spCurTask->ReExecute())
        {
            m_taskMap[m_curTaskLevel].push_front(m_spCurTaskBk);
            m_spCurTaskBk = NULL;
        }
        m_spCurTask->StopTask(false);
    }
}

void CTaskThread::SendTask(const std::shared_ptr<CTask>& task, __int32 taskLevel)
{
    if (task == NULL || taskLevel < 1 || m_hasExitSignal)
    {
        return;
    }

    PostTask(task, taskLevel);
    
    while (!task->HasExecuted())
    {
        std::chrono::milliseconds dura(50);
        std::this_thread::sleep_for(dura);
    }
}

void CTaskThread::WorkThread()
{
    //有退出信号，但是队列中有等待任务时仍然进入循环，在循环内处理多余任务
    while (!m_hasExitSignal || HasTask())
    {
        //如果当前有退出信号，则清空等待队列同时把当前任务停止
        if (m_hasExitSignal)
        {
            if (m_spCurTask != NULL)
            {
                m_spCurTask->StopTask(false);
            }
            std::unique_lock<std::mutex> lock(m_mutex);
            m_taskMap.clear();
        }
        //如果当前有任务
        if (m_spCurTask != NULL)
        {
            m_spCurTask->DoTask();
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            //一旦有退出信号则将当前任务置为空
            if (m_hasExitSignal)
            {
                m_spCurTask = NULL;
            }
            else
            {
                //取任务，没有任务则m_spCurTask变为空，有任务则填充m_spCurTask并根据剩余情况删除原有队列节点
                PopToCurTask();
            }
        }

        //无退出信号，且在弹出任务之后当前任务仍然为空时跳过
        if (!m_hasExitSignal && m_spCurTask == NULL)
        {
            std::chrono::milliseconds dura(50);
            std::this_thread::sleep_for(dura);
        }
    }
}

void CTaskThread::StopAllTaskUnlock()
{
    for (auto itListTask = m_taskMap.begin(); itListTask != m_taskMap.end(); ++itListTask)
    {
        auto listTask = itListTask->second;
        for (auto itTask = listTask.begin(); itTask != listTask.end(); ++itTask)
        {
            (*itTask)->StopTask(false);
        }
    }
    //如果有还未执行完毕的任务
    if (m_spCurTask != NULL)
    {
        m_spCurTask->StopTask(false);
    }
}

bool CTaskThread::HasTaskUnlock()
{
    return !m_taskMap.empty();
}

bool CTaskThread::HasTask()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return HasTaskUnlock();
}

void CTaskThread::PopToCurTask()
{
    if (HasTaskUnlock())
    {
        //如果有任务，取出最后一个优先级的队列任务集合
        auto itTaskList = --(m_taskMap.end());
        std::list<std::shared_ptr<CTask>>& listTask = itTaskList->second;
        //将最后一个优先级队列的首个任务取出
        m_spCurTask = listTask.front();
        m_spCurTaskBk.reset(m_spCurTask->Clone());
        m_curTaskLevel = itTaskList->first;
        //只要有集合就必须有任务
        if (m_spCurTask == NULL)
        {
            assert(0);
        }
        listTask.pop_front();
        //如果该级别队列中没有任务则删除该级别在map中的节点
        if (listTask.empty())
        {
            m_taskMap.erase(itTaskList);
        }
    }
    else
    {
        m_spCurTask = NULL;
        m_spCurTaskBk = NULL;
        m_curTaskLevel = 0;
    }
}

void CTaskThread::StopTask(__int32 taskId, bool ifChoke, __int32 taskLevel)
{
    if (taskLevel != 0)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto itListTask = m_taskMap.find(taskLevel);
        if (itListTask != m_taskMap.end())
        {
            StopTaskInList(itListTask->second, taskId, ifChoke);
        }
    }
    else
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (auto itListTask = m_taskMap.begin(); itListTask != m_taskMap.end(); ++itListTask)
        {
            StopTaskInList(itListTask->second, taskId, ifChoke);
        }
    }
}

void CTaskThread::StopTaskInList(const std::list<std::shared_ptr<CTask>>& taskList, __int32 taskId, bool ifChoke)
{
    for (auto itTask = taskList.begin(); itTask != taskList.end(); ++itTask)
    {
        if ((*itTask)->GetTaskId() == taskId)
        {
            (*itTask)->StopTask(ifChoke);
        }
    }
}

void CTaskThread::StopAllTask()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    StopAllTaskUnlock();
}

void CTaskThread::GetWaitTaskCount(std::map<__int32, __int32>& taskCountMap)
{
    taskCountMap.clear();
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (auto itTaskList = m_taskMap.begin(); itTaskList != m_taskMap.end(); ++itTaskList)
        {
            taskCountMap[itTaskList->first] = itTaskList->second.size();
        }
    }
}