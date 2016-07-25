#include <SDKDDKVer.h>
#include "CszOneValue.h"

CszOneValue::CszOneValue(){

}

CszOneValue::CszOneValue(int position,CstrValue strValue){
	this->position = position;
	this->strValue = strValue;
}

CszOneValue::~CszOneValue(){

}