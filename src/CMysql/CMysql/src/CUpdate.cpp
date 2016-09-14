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

CUpdate CUpdate::operator[](CString Field){
	strCurrentField = Field;
	return *this;
}