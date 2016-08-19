#pragma once
#include "CStopWatch/CStopWatchAPI.h"
#include "CAceTCPServerMacro.h"

class CAceTCPServerAPI CDeleteMapWatch : public CStopWatch{
public:
	void Do(void *pDo);
};

class CAceTCPServer;
typedef struct WatchPac{
	CAceTCPServer* pThis;
	int CheckKeyServer;
}WatchPac;