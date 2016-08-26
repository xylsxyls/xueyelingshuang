#pragma once
#include "CStopWatch/CStopWatchAPI.h"
#include "CAceHttpClientMacro.h"

class CAceHttpClientAPI CDeleteMapWatch : public CStopWatch{
public:
	void Do(void *pDo,BOOL* nDelete);
};

class CAceHttpClient;
class ACE_SOCK_Stream;
typedef struct WatchPac{
	CAceHttpClient* pThis;
	int CheckKeyClient;
	ACE_SOCK_Stream* ppeer;
	BOOL* pifPeerExist;
}WatchPac;