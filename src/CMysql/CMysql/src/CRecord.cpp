#include <SDKDDKVer.h>
#include "CRecord.h"

CRecord::CRecord(){
	
}

CRecord::CRecord(const CRecord& record){
	this->mapValue = record.mapValue;
}

CRecord CRecord::operator=(const CRecord& record){
	this->mapValue = record.mapValue;
	return *this;
}

CValue CRecord::operator[](CString Field){
	this->strCurrentField = Field;
	return mapValue[Field];
}

CString CRecord::ToCString(){
	CString strResult;
	for(auto it = mapValue.begin();it != mapValue.end();it++){
		strResult = strResult + it->first + "=" + it->second.strValue + ",";
	}
	strResult.Delete(strResult.GetLength() - 1,1);
	return strResult;
}