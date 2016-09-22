#include <SDKDDKVer.h>
#include "CRecord.h"
#include "CTable.h"

CRecord::CRecord(){
	
}

CRecord::CRecord(CTable* pTable){
	this->pTable = pTable;
}

CRecord::CRecord(const CRecord& record){
	this->mapValue = record.mapValue;
}

CRecord CRecord::operator=(const CRecord& record){
	this->mapValue = record.mapValue;
	return *this;
}

CRecord& CRecord::operator[](CString Field){
	this->strCurrentField = Field;
	return *this;
}

CRecord CRecord::operator=(const CValue& value){
	mapValue[strCurrentField] = value;
	return *this;
}

CAttri* CRecord::operator->(){
	return &(pTable->mapAttri[strCurrentField]);
}

CValue CRecord::toValue(){
	return mapValue[strCurrentField];
}

CString CRecord::ToCString(){
	CString strResult;
	for(auto it = mapValue.begin();it != mapValue.end();it++){
		strResult = strResult + it->first + "=" + it->second.strValue + ",";
	}
	strResult.Delete(strResult.GetLength() - 1,1);
	return strResult;
}