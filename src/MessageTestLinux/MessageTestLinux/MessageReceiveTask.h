#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class MessageTestLinux;

class MessageReceiveTask : public CTask
{
public:
    MessageReceiveTask();

public:
    void DoTask();

    void StopTask();

    void setParam(MessageTestLinux* client);

private:
    MessageTestLinux* m_client;
    std::atomic<bool> m_exit;
};