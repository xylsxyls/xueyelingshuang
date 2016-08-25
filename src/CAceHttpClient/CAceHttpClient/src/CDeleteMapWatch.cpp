#include <SDKDDKVer.h>
#include "CDeleteMapWatch.h"
#include "CAceHttpClient.h"

void CDeleteMapWatch::Do(void *pDo,BOOL* nDelete){
	WatchPac package = *(WatchPac*)pDo;
	delete (WatchPac*)pDo;
	package.ppeer->close();
	delete package.ppeer;
	*(package.pifPeerExist) = 0;
	package.pThis->DeleteMap(package.CheckKeyClient);
	return;
}