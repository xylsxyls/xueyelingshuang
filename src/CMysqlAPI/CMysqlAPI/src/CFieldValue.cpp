#include "CFieldValue.h"

CFieldValue::CFieldValue(){
	strValue = "";
	nValue = 0;
	CompareFlag = "=";
}

CFieldValue::CFieldValue(CString strValue,CField Field){
	SetValue(strValue,"=",&Field);
}

CFieldValue::CFieldValue(int nValue,CField Field){
	SetValue(nValue,"=",&Field);
}

CFieldValue::CFieldValue(CField Field,CString strValue){
	SetValue(strValue,"=",&Field);
}

CFieldValue::CFieldValue(CField Field,int nValue){
	SetValue(nValue,"=",&Field);
}

CFieldValue::CFieldValue(CField Field,CString CompareFlag,CString strValue){
	SetValue(strValue,CompareFlag,&Field);
}

CFieldValue::CFieldValue(CField Field,CString CompareFlag,int nValue){
	SetValue(nValue,CompareFlag,&Field);
}

BOOL CFieldValue::SetValue(CField Field,CString strValue){
	return SetValue(strValue,"=",&Field);
}

BOOL CFieldValue::SetValue(CField Field,int nValue){
	return SetValue(nValue,"=",&Field);
}

BOOL CFieldValue::SetValue(CString strValue,CString CompareFlag,CField *pField){
	this->Field = *pField;
	this->nValue = 0;
	if(pField->Type == "integer") this->strValue = strValue;
	//如果类型为空，说明没有给类型，那么根据传入的值进行判断
	if(pField->Type == "varchar" || pField->Type == "") this->strValue = "'" + strValue + "'";
	
	this->CompareFlag = CompareFlag;

	if(Field.Type == "integer"){
		this->nValue = atoi(strValue);
		return 0;
	}
	return 1;
}

BOOL CFieldValue::SetValue(int nValue,CString CompareFlag,CField *pField){
	this->Field = *pField;
	this->nValue = nValue;
	this->strValue = "";
	this->CompareFlag = CompareFlag;

	char buff[20] = {};
	sprintf(buff,"%d",nValue);
	if(pField->Type == "varchar") this->strValue = strValue + "'" + buff + "'";
	//如果类型为空，说明没有给类型，那么根据传入的值进行判断
	if(pField->Type == "integer" || pField->Type == "") this->strValue = buff;

	//含自动转换，以Field的类型定义为准
	if(Field.Type == "varchar") return 0;
	return 1;
}