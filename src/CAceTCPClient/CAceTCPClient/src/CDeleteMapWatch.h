#pragma once
#include "CStopWatch/CStopWatchAPI.h"
#include "CAceTCPClientMacro.h"

class CAceTCPClientAPI CDeleteMapWatch : public CStopWatch{
public:
	void Do(void *pDo);
};

class CAceTCPClient;
typedef struct WatchPac{
	CAceTCPClient* pThis;
	int CheckKeyClient;
}WatchPac;