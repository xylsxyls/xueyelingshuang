#include <SDKDDKVer.h>
#include "CTypeValue.h"
#include "Cjson.h"
#include "CszValue.h"
#include "CstrValue.h"

class CstrValue;
class CszValue;
class Cjson;

CTypeValue::CTypeValue(){
	type = "";
	pClass = NULL;
}

void CTypeValue::free(){
	if(type == "class CstrValue"){
		if(this->pClass != NULL){
			delete (CstrValue *)(this->pClass);
			this->pClass = NULL;
		}
	}
	if(type == "class CszValue"){
		if(this->pClass != NULL){
			delete (CszValue *)(this->pClass);
			this->pClass = NULL;
		}
	}
	if(type == "class Cjson"){
		if(this->pClass != NULL){
			delete (Cjson *)(this->pClass);
			this->pClass = NULL;
		}
	}
}

CTypeValue::~CTypeValue(){
	
}