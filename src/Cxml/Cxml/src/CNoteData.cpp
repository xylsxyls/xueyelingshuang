#include <SDKDDKVer.h>
#include "CNoteData.h"

CNoteData::CNoteData(){
	nPosition = 0;
}

CNoteData::CNoteData(int nPosition,CString strNoteData){
	this->nPosition = nPosition;
	this->strNoteData = strNoteData;
}