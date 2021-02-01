#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SearchPathTask : public CTask
{
public:
    SearchPathTask();

public:
    void DoTask();

    void setParam();

private:
    std::atomic<bool> m_exit;
};