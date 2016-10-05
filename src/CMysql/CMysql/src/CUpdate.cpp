#include <SDKDDKVer.h>
#include "CUpdate.h"

CUpdate::CUpdate(){

}

CUpdate::CUpdate(const CUpdate& upd){
	mapValue = upd.mapValue;
	strCurrentField = upd.strCurrentField;
}

CUpdate CUpdate::operator=(const CUpdate& upd){
	mapValue = upd.mapValue;
	strCurrentField = upd.strCurrentField;
	return *this;
}

CUpdate CUpdate::operator=(const CValue& value){
	mapValue[strCurrentField] = value;
	return *this;
}

CUpdate& CUpdate::operator[](CString Field){
	strCurrentField = Field;
	return *this;
}

CString CUpdate::toCString(){
	CString strResult;
	for(auto it = mapValue.begin();it != mapValue.end();it++){
		strResult = strResult + it->first + "=" + it->second.strValue + ",";
	}
	strResult.Delete(strResult.GetLength() - 1,1);
	return strResult;
}