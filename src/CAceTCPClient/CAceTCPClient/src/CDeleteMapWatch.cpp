#include <SDKDDKVer.h>
#include "CDeleteMapWatch.h"
#include "CAceTCPClient.h"

void CDeleteMapWatch::Do(void *pDo){
	WatchPac package = *(WatchPac*)pDo;
	delete (WatchPac*)pDo;
	package.pThis->DeleteMap(package.CheckKeyClient);
	delete this;
	return;
}