#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class MessageTestLinux;

class MessageRecoverTask : public CTask
{
public:
    MessageRecoverTask();

public:
    void DoTask();

    void StopTask();

    void setParam(MessageTestLinux* client);

private:
    MessageTestLinux* m_client;
    std::atomic<bool> m_exit;
};