#include "CFieldType.h"

CFieldType::CFieldType(){
	field = "";
	num = -1;
}

CFieldType::CFieldType(string field,int num){
	this->field = field;
	this->num = num;
}