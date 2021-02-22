#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class MessageTestLinux;

class AddStringTask : public CTask
{
public:
    AddStringTask();

public:
    void DoTask();

    void StopTask();

    void setParam(MessageTestLinux* client);

private:
    MessageTestLinux* m_client;
    std::atomic<bool> m_exit;
};