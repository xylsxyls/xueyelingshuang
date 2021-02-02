#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SearchFileTask : public CTask
{
public:
    SearchFileTask();

public:
    void DoTask();

    void setParam();

private:
    
};