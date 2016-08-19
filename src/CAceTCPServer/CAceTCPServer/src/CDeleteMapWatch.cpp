#include <SDKDDKVer.h>
#include "CDeleteMapWatch.h"
#include "CAceTCPServer.h"

void CDeleteMapWatch::Do(void *pDo){
	WatchPac package = *(WatchPac*)pDo;
	delete (WatchPac*)pDo;
	package.pThis->DeleteMap(package.CheckKeyServer);
	delete this;
	return;
}