#include <SDKDDKVer.h>
#include "CstrValue.h"

CstrValue::CstrValue(){

}

CstrValue::CstrValue(CString strValue){
	//如果有引号说明是字符串
	if(strValue.Find('\"') >= 0){
		int nLeft = strValue.Find('\"');
		int nRight = strValue.Find('\"',nLeft + 1);
		this->strValue = strValue.Mid(nLeft + 1,nRight - nLeft - 1);
		type = 1;
	}
	//如果没有引号但有小数点说明是小数
	else if(strValue.Find('.') >= 0){
		dValue = atof(strValue);
		this->strValue.Format("%lf",dValue);
		type = 3;
	}
	//其余只有是整数的情况
	else{
		nValue = atoi(strValue);
		this->strValue.Format("%d",nValue);
		type = 2;
	}
}

CstrValue::~CstrValue(){

}