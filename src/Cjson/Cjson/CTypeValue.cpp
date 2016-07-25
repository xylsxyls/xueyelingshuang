#include <SDKDDKVer.h>
#include "CTypeValue.h"


CTypeValue::CTypeValue(){
	type = "";
	pClass = NULL;
}

CTypeValue::CTypeValue(const CstrValue& Class){
	type = typeid(Class).name();
	CstrValue *pNewClass = new CstrValue;
	*pNewClass = Class;
	pClass = pNewClass;
}

CTypeValue::CTypeValue(const CszValue& Class){
	type = typeid(Class).name();
	CszValue *pNewClass = new CszValue;
	*pNewClass = Class;
	pClass = pNewClass;
}
CTypeValue::CTypeValue(const Cjson& Class){
	type = typeid(Class).name();
	Cjson *pNewClass = new Cjson;
	*pNewClass = Class;
	pClass = pNewClass;
}

CTypeValue::CTypeValue(const CTypeValue& x){
	type = x.type;
	if(type == "class CstrValue"){
		//pClass必须新new一个
		CstrValue *pNewClass = new CstrValue;
		*pNewClass = *((CstrValue *)(x.pClass));
		pClass = pNewClass;
	}
	if(type == "class CszValue"){
		//pClass必须新new一个
		CszValue *pNewClass = new CszValue();
		*pNewClass = *((CszValue *)(x.pClass));
		pClass = pNewClass;
	}
	if(type == "class Cjson"){
		//pClass必须新new一个
		Cjson *pNewClass = new Cjson();
		*pNewClass = *((Cjson *)(x.pClass));
		pClass = pNewClass;
	}
}

CTypeValue CTypeValue::operator = (const CTypeValue& x){
	type = x.type;
	if(type == "class CstrValue"){
		//pClass必须新new一个
		CstrValue *pNewClass = new CstrValue;
		*pNewClass = *((CstrValue *)(x.pClass));
		pClass = pNewClass;
	}
	if(type == "class CszValue"){
		//pClass必须新new一个
		CszValue *pNewClass = new CszValue();
		*pNewClass = *((CszValue *)(x.pClass));
		pClass = pNewClass;
	}
	if(type == "class Cjson"){
		//pClass必须新new一个
		Cjson *pNewClass = new Cjson();
		*pNewClass = *((Cjson *)(x.pClass));
		pClass = pNewClass;
	}
	return *this;
}

CTypeValue::~CTypeValue(){
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