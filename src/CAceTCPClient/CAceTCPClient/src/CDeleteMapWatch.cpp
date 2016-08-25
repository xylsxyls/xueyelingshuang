#include <SDKDDKVer.h>
#include "CDeleteMapWatch.h"
#include "CAceTCPClient.h"

void CDeleteMapWatch::Do(void *pDo,BOOL* nDelete){
	WatchPac package = *(WatchPac*)pDo;
	delete (WatchPac*)pDo;
	package.pThis->DeleteMap(package.CheckKeyClient);
	*nDelete = 1;
	delete this;
	return;
}