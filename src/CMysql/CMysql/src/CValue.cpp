#include <SDKDDKVer.h>
#include "CValue.h"
#include "CAttri.h"

CValue::CValue(){
	this->Type = 0;
	this->nValue = 0;
	this->dValue = 0;
	this->strValue = "";
}

CValue::CValue(CString strValue){
	this->Type = 1;
	this->nValue = 0;
	this->dValue = 0;
	this->strValue = "'" + strValue + "'";
}

CValue::CValue(char* szValue){
	this->Type = 1;
	this->nValue = 0;
	this->dValue = 0;
	this->strValue = this->strValue + "'" + szValue + "'";
}

CValue::CValue(string strValue){
	this->Type = 1;
	this->nValue = 0;
	this->dValue = 0;
	this->strValue = ("'" + strValue + "'").c_str();
}

CValue::CValue(int nValue){
	this->Type = 2;
	this->nValue = nValue;
	this->dValue = 0;
	this->strValue.Format("%d",nValue);
}

CValue::CValue(double dValue){
	this->Type = 3;
	this->nValue = 0;
	this->dValue = dValue;
	this->strValue.Format("%lf",dValue);
}

CValue::CValue(const CValue& value){
	this->Type = value.Type;
	this->nValue = value.nValue;
	this->dValue = value.dValue;
	this->strValue = value.strValue;
}

CValue CValue::operator = (const CValue& value){
	this->Type = value.Type;
	this->nValue = value.nValue;
	this->dValue = value.dValue;
	this->strValue = value.strValue;
	return *this;
}

bool CValue::operator== (const CValue& value){
	return this->Type == value.Type &&
		this->nValue == value.nValue &&
		this->dValue == value.dValue &&
		this->strValue == value.strValue;
}

bool CValue::operator!= (const CValue& value){
	return this->Type != value.Type &&
		this->nValue != value.nValue &&
		this->dValue != value.dValue &&
		this->strValue != value.strValue;
}