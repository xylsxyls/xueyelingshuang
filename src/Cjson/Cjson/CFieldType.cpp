#include <SDKDDKVer.h>
#include "CFieldType.h"

CFieldType::CFieldType(){
	field = "";
	num = -1;
}

CFieldType::CFieldType(CString field,int num){
	this->field = field;
	this->num = num;
}