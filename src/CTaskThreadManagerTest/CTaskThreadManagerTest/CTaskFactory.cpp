#include "stdafx.h"
#include "CTaskFactory.h"
#include "CRandom/CRandomAPI.h"

std::shared_ptr<CTask> CTaskFactory::CreateTask(int32_t taskType, int32_t taskId)
{
    std::shared_ptr<CTask> spTask;
    switch (taskType)
    {
    case PRINT_TASK_1:
    {
        spTask.reset(new CPrintTask1(taskId));
        break;
    }
    case PRINT_TASK_2:
    {
        spTask.reset(new CPrintTask2(taskId));
        break;
    }
    case PRINT_TASK_3:
    {
        spTask.reset(new CPrintTask3(taskId));
        break;
    }
    case TEST_TASK_1:
    {
        spTask.reset(new CTestTask1(taskId));
        break;
    }
    case TEST_TASK_2:
    {
        spTask.reset(new CTestTask2(taskId));
        break;
    }
    case TEST_TASK_3:
    {
        spTask.reset(new CTestTask3(taskId));
        break;
    }
    case TEST_TASK_4:
    {
        spTask.reset(new CTestTask4(taskId));
        break;
    }
    case TEST_TASK_5:
    {
        spTask.reset(new CTestTask5(taskId));
        break;
    }
    case TEST_TASK_6:
    {
        spTask.reset(new CTestTask6(taskId));
        break;
    }
    case TEST_TASK_7:
    {
        spTask.reset(new CTestTask7(taskId));
        break;
    }
    case TEST_TASK_8:
    {
        spTask.reset(new CTestTask8(taskId));
        break;
    }
    case TEST_TASK_9:
    {
        spTask.reset(new CTestTask9(taskId));
        break;
    }
    case TEST_TASK_10:
    {
        spTask.reset(new CTestTask10(taskId));
        break;
    }
    default:
        break;
    }
    return spTask;
}

CPrintTask1::CPrintTask1(int32_t taskId):
CTask(taskId),
m_hasExitSignal(false)
{

}

void CPrintTask1::DoTask()
{
    printf("DoTaskBegin1\n");
	int i = -1;
    while (i++ != 4)
    {
        if (m_hasExitSignal)
        {
            break;
        }
		RCSend("DoTask1_%d, taskId = %d", i + 1, GetTaskId());
        std::chrono::milliseconds dura(1000);
        std::this_thread::sleep_for(dura);
    }

    printf("DoTaskEnd1\n");
}

void CPrintTask1::StopTask()
{
    m_hasExitSignal = true;
    printf("StopTask1\n");
    return;
}
bool CPrintTask1::ReExecute()
{
    printf("ReExecute1\n");
    return true;
}

CTask* CPrintTask1::Clone()
{
    return new CPrintTask1(GetTaskId());
}

CPrintTask2::CPrintTask2(int32_t taskId) :
CTask(taskId),
m_hasExitSignal(false)
{

}

void CPrintTask2::DoTask()
{
    printf("DoTaskBegin2\n");
    int i = -1;
    while (i++ != 4)
    {
        if (m_hasExitSignal)
        {
            break;
        }
		RCSend("DoTask2_%d, taskId = %d", i + 1, GetTaskId());
        std::chrono::milliseconds dura(1000);
        std::this_thread::sleep_for(dura);
    }

    printf("DoTaskEnd2\n");
}

void CPrintTask2::StopTask()
{
    m_hasExitSignal = true;
    printf("StopTask2\n");
    return;
}

bool CPrintTask2::ReExecute()
{
    printf("ReExecute2\n");
    return true;
}

CTask* CPrintTask2::Clone()
{
    return new CPrintTask2(GetTaskId());
}

CPrintTask3::CPrintTask3(int32_t taskId) :
CTask(taskId),
m_hasExitSignal(false)
{

}

void CPrintTask3::DoTask()
{
    printf("DoTaskBegin3\n");
    int i = -1;
    while (i++ != 4)
    {
        if (m_hasExitSignal)
        {
            break;
        }
		RCSend("DoTask3_%d, taskId = %d", i + 1, GetTaskId());
        std::chrono::milliseconds dura(1000);
        std::this_thread::sleep_for(dura);
    }

    printf("DoTaskEnd3\n");
}

void CPrintTask3::StopTask()
{
    m_hasExitSignal = true;
    printf("StopTask3\n");
    return;
}

bool CPrintTask3::ReExecute()
{
    printf("ReExecute3\n");
    return true;
}

CTask* CPrintTask3::Clone()
{
    return new CPrintTask3(GetTaskId());
}

CTestTask1::CTestTask1(int32_t taskId) :
CTask(taskId),
m_hasExitSignal(false)
{

}

void CTestTask1::DoTask()
{
    int32_t i = 10;
    while (i-- != 0)
    {
        if (m_hasExitSignal)
        {
            break;
        }
		RCSend("task1");
        //生成任务号
        int32_t taskId = CRandom::Int(1, 10000);
        //生成任务等级
        int32_t taskLevel = CRandom::Int(1, 10);
        //生成线程号
        int32_t threadId = CRandom::Int(1, 20);
        //随机选择任务
        int32_t taskType = CRandom::Int(0, 9);
        //是否停止所有任务
        int32_t ifStopAll = CRandom::Int(1, 10);
        //发送任务
        auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
        if (taskThread == NULL)
        {
            continue;
        }
        taskThread->PostTask(CTaskFactory::CreateTask(TEST_TASK_1 + taskType, taskId), taskLevel);
    }
}

void CTestTask1::StopTask()
{
    m_hasExitSignal = true;
    return;
}

bool CTestTask1::ReExecute()
{
    return true;
}

CTask* CTestTask1::Clone()
{
    return new CTestTask1(GetTaskId());
}

CTestTask2::CTestTask2(int32_t taskId):
CTestTask1(taskId)
{

}

CTestTask3::CTestTask3(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask4::CTestTask4(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask5::CTestTask5(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask6::CTestTask6(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask7::CTestTask7(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask8::CTestTask8(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask9::CTestTask9(int32_t taskId) :
CTestTask1(taskId)
{

}

CTestTask10::CTestTask10(int32_t taskId) :
CTestTask1(taskId)
{

}