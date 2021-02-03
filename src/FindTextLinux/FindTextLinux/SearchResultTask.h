#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SearchPathTask;

class SearchResultTask : public CTask
{
public:
    SearchResultTask();

public:
    void DoTask();

    void setParam(const std::string& result, SearchPathTask* task);

private:
    std::string m_result;
    SearchPathTask* m_task;
};