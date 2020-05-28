#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "D:\\SendToMessageTest.h"
#include <atomic>

class CTaskFactory
{
public:
    //可能因为内存不够的情况会创建出空的只能指针，但是仍然可以使用PostTask和SendTask，会在内部过滤
    static std::shared_ptr<CTask> CreateTask(int32_t taskType, int32_t taskId);
};

enum
{
    PRINT_TASK_1,
    PRINT_TASK_2,
    PRINT_TASK_3,
    TEST_TASK_1,
    TEST_TASK_2,
    TEST_TASK_3,
    TEST_TASK_4,
    TEST_TASK_5,
    TEST_TASK_6,
    TEST_TASK_7,
    TEST_TASK_8,
    TEST_TASK_9,
    TEST_TASK_10,
};

class CPrintTask1 : public CTask
{
public:
    CPrintTask1(int32_t taskId);
    virtual void DoTask();
    virtual void StopTask();
    virtual bool ReExecute();
    virtual CTask* Clone();

private:
	std::atomic<bool> m_hasExitSignal;
};

class CPrintTask2 : public CTask
{
public:
    CPrintTask2(int32_t taskId);
    virtual void DoTask();
    virtual void StopTask();
    virtual bool ReExecute();
    virtual CTask* Clone();

private:
	std::atomic<bool> m_hasExitSignal;
};

class CPrintTask3 : public CTask
{
public:
    CPrintTask3(int32_t taskId);
    virtual void DoTask();
    virtual void StopTask();
    virtual bool ReExecute();
    virtual CTask* Clone();

private:
	std::atomic<bool> m_hasExitSignal;
};

class CTestTask1 : public CTask
{
public:
    CTestTask1(int32_t taskId);
    virtual void DoTask();
    virtual void StopTask();
    virtual bool ReExecute();
    virtual CTask* Clone();

private:
    std::atomic<bool> m_hasExitSignal;
};

class CTestTask2 : public CTestTask1
{
public:
    CTestTask2(int32_t taskId);
};

class CTestTask3 : public CTestTask1
{
public:
    CTestTask3(int32_t taskId);
};

class CTestTask4 : public CTestTask1
{
public:
    CTestTask4(int32_t taskId);
};

class CTestTask5 : public CTestTask1
{
public:
    CTestTask5(int32_t taskId);
};

class CTestTask6 : public CTestTask1
{
public:
    CTestTask6(int32_t taskId);
};

class CTestTask7 : public CTestTask1
{
public:
    CTestTask7(int32_t taskId);
};

class CTestTask8 : public CTestTask1
{
public:
    CTestTask8(int32_t taskId);
};

class CTestTask9 : public CTestTask1
{
public:
    CTestTask9(int32_t taskId);
};

class CTestTask10 : public CTestTask1
{
public:
    CTestTask10(int32_t taskId);
};