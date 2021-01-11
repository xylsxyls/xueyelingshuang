#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class MessageTestLinux;

class MessagePidTask : public CTask
{
public:
    MessagePidTask();

public:
    void DoTask();

    void StopTask();

    void setParam(int32_t pid, MessageTestLinux* client);

private:
    int32_t m_pid;
    MessageTestLinux* m_client;
    std::atomic<bool> m_exit;
};
