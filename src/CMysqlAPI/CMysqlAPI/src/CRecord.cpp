#include "CRecord.h"

CRecord::CRecord(){
	
}

CRecord::CRecord(CField x){
	*this = x;
}

CRecord::CRecord(CFieldValue x){
	*this = x;
}

CRecord CRecord::operator = (CField x){
	CRecord result;
	*this = result;
	this->vecFieldValue.push_back(CFieldValue(x,""));
	return *this;
}

CRecord CRecord::operator = (CFieldValue x){
	CRecord result;
	*this = result;
	this->vecFieldValue.push_back(x);
	return *this;
}

CRecord operator + (CRecord x,CFieldValue y){
	CRecord result = x;
	result.vecFieldValue.push_back(y);
	//通过的情况为条件向量为空，条件向量=字段值向量-1，不通过则为非
	if(!((result.vecCondition.size() == 0) || (result.vecCondition.size() == result.vecFieldValue.size() - 1))){
		CRecord empty;
		return empty;
	}
	return result;
}

CRecord operator + (CFieldValue x,CRecord y){
	CRecord result = y;
	result.vecFieldValue.push_back(x);
	//通过的情况为条件向量为0，不通过则为非
	if(!(result.vecCondition.size() == 0)){
		CRecord empty;
		return empty;
	}
	return result;
}

CRecord operator + (CRecord x,CRecord y){
	CRecord result = x;
	int i = -1;
	while(i++ != y.vecFieldValue.size() - 1){
		result.vecFieldValue.push_back(y.vecFieldValue.at(i));
	}
	i = -1;
	while(i++ != y.vecCondition.size() - 1){
		result.vecCondition.push_back(y.vecCondition.at(i));
	}
	//通过的情况为条件向量为0，不通过则为非
	if(!(result.vecCondition.size() == 0)){
		CRecord empty;
		return empty;
	}
	return result;
}

CRecord operator + (CRecord x,CString y){
	CRecord result = x;
	if(y != ",") result.vecCondition.push_back(y);
	//通过的情况为字段值向量=条件向量，不通过则为非
	if((!(result.vecCondition.size() == result.vecFieldValue.size())) || (y != "and" && y != "or")){
		CRecord empty;
		return empty;
	}
	return result;
}

CRecord operator + (CFieldValue x,CString y){
	CRecord result = x;
	return result + y;
}

CRecord operator + (CFieldValue x,CFieldValue y){
	CRecord result;
	result.vecFieldValue.push_back(x);
	result.vecFieldValue.push_back(y);
	return result;
}

CRecord operator + (CRecord x,CField y){
	CRecord result = x;
	result.vecFieldValue.push_back(CFieldValue(y,""));
	return result;
}

CRecord operator + (CField x,CRecord y){
	return y + x;
}

CRecord operator + (CField x,CField y){
	CRecord result;
	return result + x + y;
}

CRecord* CRecord::SetValue(vector<CField> vecField,...){
	va_list parameterlist;
	va_start(parameterlist,vecField);

	//先清空，CRecord中只有一个向量成员
	vector<CFieldValue> empty;
	vecFieldValue = empty;

	int i = -1;
	//将变成参数中的char*和向量中的字段属性一一对应存入字段值中，再加到记录中
	while(i++ != vecField.size() - 1) *this = *this + CFieldValue(va_arg(parameterlist,char*),vecField.at(i));
	
	va_end(parameterlist);
	return this;
}

BOOL CRecord::IsConditionRecord(){
	int x = this->vecCondition.size();
	int y = this->vecFieldValue.size();
	return (this->vecFieldValue.size() >= 1) && (this->vecCondition.size() == this->vecFieldValue.size() - 1);
}

BOOL CRecord::IsSetRecord(){
	return (this->vecFieldValue.size() >= 1) && (this->vecCondition.size() == 0);
}

CString CRecord::OutPutFieldName(){
	CString SQL = "";
	int i = -1;
	while(i++ != vecFieldValue.size() - 1){
		if(i != 0) SQL = SQL + ",";
		SQL = SQL + vecFieldValue.at(i).Field.FieldName;
	}
	return SQL;
}

CString CRecord::RecordToCString(){
	CString SQL = "";
	int i = -1;
	while(i++ != vecFieldValue.size() - 1){
		if(i != 0){
			//如果是设置记录
			if(IsSetRecord() == 1) SQL = SQL + ",";
			//如果是条件记录
			if(IsConditionRecord() == 1) SQL = SQL + " " + vecCondition.at(i - 1) + " ";
		}
		SQL = SQL + vecFieldValue.at(i).Field.FieldName + vecFieldValue.at(i).CompareFlag + vecFieldValue.at(i).strValue;
	}
	return SQL;
}

CRecord CRecord::ConditionToSet(){
	CRecord result = *this;
	result.vecCondition.clear();
	return result;
}