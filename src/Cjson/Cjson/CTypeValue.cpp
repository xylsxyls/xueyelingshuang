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

CTypeValue CTypeValue::operator[] (CString field){
	if(type == "class CstrValue"){
		
	}
	if(type == "class CszValue"){
		
	}
	if(type == "class Cjson"){
		return ((Cjson*)pClass)->mapdata[field];
	}
	return *this;
}

CTypeValue CTypeValue::operator[] (int num){
	if(type == "class CstrValue"){
		
	}
	if(type == "class CszValue"){
		return ((CszValue*)pClass)->vecszValue.at(num);
	}
	if(type == "class Cjson"){
		
	}
	return *this;
}

CstrValue CTypeValue::toValue(){
	if(type == "class CstrValue"){
		return *((CstrValue *)pClass);
	}
	CstrValue error;
	error.dValue = -1;
	error.nValue = -1;
	error.strValue = "-1";
	error.type = -1;
	return error;
}

CszValue CTypeValue::tosz(){
	if(type == "class CszValue"){
		return *((CszValue *)pClass);
	}
	CszValue error;
	return error;
}

Cjson CTypeValue::toJson(){
	if(type == "class Cjson"){
		return *((Cjson *)pClass);
	}
	Cjson error;
	return error;
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