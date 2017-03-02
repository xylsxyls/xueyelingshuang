#include "CstrValue.h"
#include "CStringManager/CStringManagerAPI.h"

CstrValue::CstrValue(){
	type = 0;
	strValue = "";
	nValue = 0;
	dValue = 0;
}

CstrValue::CstrValue(BOOL type,string strValue,int nValue,double dValue){
	this->type = type;
	this->strValue = strValue;
	this->nValue = nValue;
	this->dValue = dValue;
}

CstrValue::CstrValue(string strValue){
	//需要改动，防止用户乱输的情况，-"123".6
	//先初始化
	type = 0;
	this->strValue = "";
	nValue = 0;
	dValue = 0;

	//先去掉左右两边的无效字符，为了防止是不可识别字符串先保存
	string strValueTemp = strValue;
	int i = -1;
	while(i++ != CStringManager::GetLength(strValueTemp) - 1){
		//遇到无效字符则跳过
		if(strValueTemp[i] == ' ' || (strValueTemp[i] == '\r' && strValueTemp[i + 1] == '\n') || strValueTemp[i] == '\n' || strValueTemp[i] == '\t') continue;
		else break;
	}
	//从有效字符开始取到末尾
	strValueTemp = CStringManager::Mid(strValueTemp, i, CStringManager::GetLength(strValueTemp) - i);
	//如果取出来是空字符串说明整条都是无效字符则说明这是不可识别字符串
	if(strValueTemp == ""){
		goto rem;
	}
	//水平翻转
	CStringManager::MakeReverse(strValueTemp);
	i = -1;
	while (i++ != CStringManager::GetLength(strValueTemp) - 1){
		//遇到无效字符则跳过
		if(strValueTemp[i] == ' ' || strValueTemp[i] == '\r' || strValueTemp[i] == '\n' || strValueTemp[i] == '\t') continue;
		else break;
	}
	//从有效字符开始取到末尾
	strValueTemp = CStringManager::Mid(strValueTemp, i, CStringManager::GetLength(strValueTemp) - i);
	//水平翻转
	CStringManager::MakeReverse(strValueTemp);

	//如果去掉头尾的无效字符之后第一个和最后一个都是引号说明是字符串
	if (strValueTemp[0] == '\"' && strValueTemp[CStringManager::GetLength(strValueTemp) - 1] == '\"'){
		type = 1;
		this->strValue = CStringManager::Mid(strValueTemp, 1, CStringManager::GetLength(strValueTemp) - 2);
	}
	//说明这里是一个空字段值，默认转成数字0
	else if(strValueTemp == "null"){
		type = 2;
		this->strValue = strValueTemp;
		this->nValue = 0;
	}
	//如果是负小数，符号在首位，且只有一个小数点
	else if ((CStringManager::Replace(strValueTemp, "-", "-") == 1 && CStringManager::Find(strValueTemp, '-') == 0) && CStringManager::Replace(strValueTemp,".", ".") == 1){
		//如果其他全部是数字则说明这是小数
		i = -1;
		while (i++ != CStringManager::GetLength(strValueTemp) - 1){
			if((strValueTemp[i] >= '0' && strValueTemp[i] <= '9') || strValueTemp[i] == '-' || strValueTemp[i] == '.') continue;
			else{
				//如果还有其他字符则说明这条字符串不可识别
				goto rem;
			}
		}
		//如果是自然退出循环则说明这是一条负小数数据
		type = 3;
		this->strValue = strValueTemp;
		dValue = atof(this->strValue.c_str());
	}
	//如果是负整数
	else if ((CStringManager::Replace(strValueTemp, "-", "-") == 1 && CStringManager::Find(strValueTemp, '-') == 0) && CStringManager::Replace(strValueTemp,".", ".") == 0){
		//如果其他全部是数字则说明这是整数
		i = -1;
		while (i++ != CStringManager::GetLength(strValueTemp) - 1){
			if((strValueTemp[i] >= '0' && strValueTemp[i] <= '9') || strValueTemp[i] == '-') continue;
			else{
				//如果还有其他字符则说明这条字符串不可识别
				goto rem;
			}
		}
		//如果是自然退出循环则说明这是一条负整数数据
		type = 2;
		this->strValue = strValueTemp;
		nValue = atoi(this->strValue.c_str());
	}
	else if (CStringManager::Replace(strValueTemp, "-", "-") == 0 && CStringManager::Replace(strValueTemp,".", ".") == 1){
		//如果其他全部是数字则说明这是小数
		i = -1;
		while (i++ != CStringManager::GetLength(strValueTemp) - 1){
			if((strValueTemp[i] >= '0' && strValueTemp[i] <= '9') || strValueTemp[i] == '.') continue;
			else{
				//如果还有其他字符则说明这条字符串不可识别
				goto rem;
			}
		}
		//如果是自然退出循环则说明这是一条正小数数据
		type = 3;
		this->strValue = strValueTemp;
		dValue = atof(this->strValue.c_str());
	}
	//如果是正整数或0
	else if (CStringManager::Replace(strValueTemp, "-", "-") == 0 && CStringManager::Replace(strValueTemp,".", ".") == 0){
		//如果其他全部是数字则说明这是整数
		i = -1;
		while (i++ != CStringManager::GetLength(strValueTemp) - 1){
			if(strValueTemp[i] >= '0' && strValueTemp[i] <= '9') continue;
			else{
				//如果还有其他字符则说明这条字符串不可识别
				goto rem;
			}
		}
		//如果是自然退出循环则说明这是一条正小数数据
		type = 2;
		this->strValue = strValueTemp;
		nValue = atoi(this->strValue.c_str());
	}
	//除此之外都是不可识别字符串
	else{
		goto rem;
	}

	return;

	rem:
	type = -1;
	this->strValue = strValue;
	return;

	/*
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
	//其余只有是整数的情况和运算表达式的情况
	else{
		int i = -1;
		strValue.Replace(" " ,"");
		strValue.Replace("\n","");
		strValue.Replace("\r","");
		strValue.Replace("\t","");
		while(i++ != strValue.GetLength() - 1){
			if((strValue[i] < 48 || strValue[i] > 57) && strValue[i] != '.' && strValue[i] != '-') break;
		}
		//如果不等说明出现了其他符号，则是运算表达式，不能处理
		if(i != strValue.GetLength()){
			this->strValue = strValue;
			type = -1;
		}
		else{
			nValue = atoi(strValue);
			this->strValue.Format("%d",nValue);
			type = 2;
		}
	}*/
}

CstrValue::CstrValue(const CstrValue& x){
	this->dValue = x.dValue;
	this->nValue = x.nValue;
	this->strValue = x.strValue;
	this->type = x.type;
}

CstrValue CstrValue::operator = (const CstrValue& x){
	this->dValue = x.dValue;
	this->nValue = x.nValue;
	this->strValue = x.strValue;
	this->type = x.type;
	return *this;
}

CstrValue::operator int(){
	return nValue;
}

CstrValue::operator string(){
	return strValue;
}

CstrValue::operator double(){
	return dValue;
}

CstrValue::~CstrValue(){

}