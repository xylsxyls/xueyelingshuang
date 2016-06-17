#include "CField.h"

CField::CField(){
	SetAttributes("","",0,0,"",0);
}

CField::CField(CString FieldName){
	SetAttributes(FieldName,"",0,0,"",0);
}

CField::CField(CString FieldName,CString Type,int Length,BOOL ifPrimaryKey){
	SetAttributes(FieldName,Type,Length,0,"",ifPrimaryKey);
}

CField::CField(CString FieldName,int nType,int Length,BOOL ifPrimaryKey){
	if(nType == 3) this->Type = "integer";
	else if(nType == 253) this->Type = "varchar";

	SetAttributes(FieldName,Type,Length,0,"",ifPrimaryKey);
}

void CField::SetAttributes(CString FieldName,int nType,int Length,BOOL Default,CString strDefault,BOOL ifPrimaryKey){
	if(nType == 3) this->Type = "integer";
	else if(nType == 253) this->Type = "varchar";

	SetAttributes(FieldName,Type,Length,Default,strDefault,ifPrimaryKey);
	return;
}

void CField::SetAttributes(CString FieldName,CString Type,int Length,BOOL Default,CString strDefault,BOOL ifPrimaryKey){
	this->FieldName = FieldName;
	if(Type == "varchar" || Type == "varchar2" || Type == "string") this->Type = "varchar";
	else if(Type == "int" || Type == "integer" || Type == "unsigned int" || Type == "unsigned long" ||  Type == "long") this->Type = "integer";
	this->Length = Length;
	char buff[20] = {}; sprintf(buff,"%d",Length); this->strLength = buff;
	this->Default = Default;
	this->nDefault = 0;
	if(Type == "int" || Type == "integer" || Type == "unsigned int" || Type == "unsigned long" ||  Type == "long") this->nDefault = atoi(strDefault);
	this->strDefault = strDefault;
	this->ifPrimaryKey = ifPrimaryKey;
	return;
}