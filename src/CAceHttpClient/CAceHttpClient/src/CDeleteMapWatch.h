#pragma once
#include "CStopWatch/CStopWatchAPI.h"

class CDeleteMapWatch : public CStopWatch{
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